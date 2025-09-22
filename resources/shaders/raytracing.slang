import common.camera;
import common.RT_common;
import common.mesh_common;
import common.lights;
import common.random;

[vk::binding(0, 0)]
public RaytracingAccelerationStructure sceneBVH;
[vk::binding(1, 0)]
public RWTexture2D resultTexture;
[vk::binding(2, 0)]
public Sampler2D textures[];

public struct Attributes
{
    public float2 barycentrics : SV_BaryCentrics;
};

[[vk::push_constant]]
cbuffer PushConstants
{
    ViewData     *viewDataPtr;
    Vertex       *vertexDataPtr;
    uint         *indexDataPtr;
    RTMeshData *commonDataPtr;
    PointLight *pointLightsPtr;

    uint maxRecursionDepth;
}

[shader("raygeneration")]
void RaygenMain()
{    
    uint3 rayIdx = DispatchRaysIndex();

    float2 ndc;
    ndc.x = (2.0f * rayIdx.x) / DispatchRaysDimensions().x - 1.0f;
    ndc.y = 1.0f - (2.0f * rayIdx.y) / DispatchRaysDimensions().y; // y inverted

    float4 pointWorldSpace = mul(viewDataPtr->inverseView, mul(viewDataPtr->inverseProjection, float4(ndc.x, ndc.y, 1.0f, 1.0f)));

    float3 originWorld = viewDataPtr->position;

    float3 dirWorld = normalize(pointWorldSpace.xyz);

    float4 color = float4(0.0f);

    RayDesc ray;
    ray.Origin = originWorld;
    ray.Direction = dirWorld;
    ray.TMin = 0.001f;
    ray.TMax = viewDataPtr->farPlane;

    float seed = BaseHash(rayIdx.xy);

    for (uint i = 0; i < maxRecursionDepth; ++i)
    {
        RayPayload payload = { float4(0, 0, 0, 1.0f), 0.0f, ray, seed, false };
        TraceRay(sceneBVH, RAY_FLAG_NONE, ~0, 0, 0, 0, ray, payload);

        if (payload.distance < 0.0f)
        {
            color += payload.color;
            break;
        }

        color += payload.color;

        if (payload.hitLight)
            break;
            
        // Can't update it just inside of closest hit shader, so need to apply these changes here
        ray.Origin = payload.ray.Origin;
        ray.Direction = payload.ray.Direction;
    }

    color = color / (color + float4(1.0f));
    color = pow(color, float4(1.0f / 2.2f));

    resultTexture.Store(rayIdx.xy, color); 
}



[shader("closesthit")]
void ClosestMain(inout RayPayload payload, in Attributes attr)
{
    Triangle triangle;
    const float3 barycentricsCoords = float3(1.0f - attr.barycentrics.x - attr.barycentrics.y, 
        attr.barycentrics.x, attr.barycentrics.y);

    RTMeshData meshData = commonDataPtr[InstanceIndex()];

    const uint primitiveIdx = PrimitiveIndex() * 3 + meshData.indexBufferOffset;
    for (int i = 0; i < 3; ++i)
    {
        const uint index = indexDataPtr[primitiveIdx + i];
        triangle.uv[i] = vertexDataPtr[meshData.vertexBufferOffset + index].UV;
        triangle.tangent[i] = vertexDataPtr[meshData.vertexBufferOffset + index].tangent;
    }

    triangle.uv[0].y = -triangle.uv[0].y;
    triangle.uv[1].y = -triangle.uv[1].y;
    triangle.uv[2].y = -triangle.uv[2].y;

    const float2 uv = barycentricsCoords.x * triangle.uv[0] 
                    + barycentricsCoords.y * triangle.uv[1] 
                    + barycentricsCoords.z * triangle.uv[2];

    const float3 pos = barycentricsCoords.x * HitTriangleVertexPosition(0) 
                     + barycentricsCoords.y * HitTriangleVertexPosition(1) 
                     + barycentricsCoords.z * HitTriangleVertexPosition(2);

    const int albedoID = meshData.materialDesc.albedoID;
    const int normalID = meshData.materialDesc.normalID;
    const int metalRoughnessID = meshData.materialDesc.metalRoughnessID;

    const float2 dUvX = GetScreenDerivativeX(uv);
    const float2 dUvY = GetScreenDerivativeY(uv);
    const float2 screenExtent = viewDataPtr->extent;

    const float4 normal = textures[normalID].SampleGrad(uv, dUvX / screenExtent.x, dUvY / screenExtent.y);
    const float4 albedo = textures[albedoID].SampleGrad(uv, dUvX / screenExtent.x, dUvY / screenExtent.y);
    const float4 metallicRoughness = textures[metalRoughnessID].SampleGrad(uv, dUvX / screenExtent.x, dUvY / screenExtent.y);


    // To make importance sampling
    const float3 lightPos = pointLightsPtr[0].position;
    const float3 hitPoint = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    payload.distance = RayTCurrent();
    payload.color = float4(0.0f);


    RayDesc shadowRay;
    shadowRay.Origin = hitPoint + normal.xyz * 0.0001f;
    shadowRay.Direction = normalize(lightPos - shadowRay.Origin);
    shadowRay.TMin = 0.001f;
    const float distanceToLight = distance(lightPos, shadowRay.Origin);
    shadowRay.TMax = distanceToLight - pointLightsPtr[0].radius;

    if (NoObstaclesRay(sceneBVH, shadowRay, 0xFF))
        payload.color = float4(CalculateColor(pointLightsPtr[0], albedo, normal, metallicRoughness, shadowRay.Origin, pos), 1.0f);

   const float3 tangent = barycentricsCoords.x * triangle.tangent[0] 
                        + barycentricsCoords.y * triangle.tangent[1] 
                        + barycentricsCoords.z * triangle.tangent[2];

   const float3 bitangent = normalize(cross(tangent, normal.xyz));

   float3 T = normalize(mul(ObjectToWorld(), float4(tangent, 0.0)).xyz);
   float3 N = normalize(mul(ObjectToWorld(), float4(normal.xyz, 0.0)).xyz);

   // Gram-Schmidt process to make vectors orthogonal back
   T = normalize(T - dot(T, N) * N);
   float3 B = normalize(cross(T, N));
   float3x3 TBN = float3x3(T, B, N);
   TBN = transpose(TBN); // important in slang

   const float3 hitPos = payload.ray.Origin + payload.ray.Direction * RayTCurrent();
   payload.ray.Origin = hitPos + normal.xyz * 0.001f;
   payload.ray.Direction = normalize(mul(TBN, RandomOnHemisphere(payload.seed, normal.xyz)));
}

float3 CalculateColor(PointLight light, float4 albedo, float4 normal, float4 metrou, float3 rayOrigin, float3 fragPos)
{
    return CalculateBRDF(light, albedo.xyz, metrou.xyz, normal.xyz, rayOrigin, fragPos);
}



[shader("miss")]
void MissMain(inout RayPayload payload)
{
    payload.color = float4(0.0f, 0.0f, 0.0f, 1.0f);
    payload.distance = -1.0f;
}

[shader("anyhit")]
void AnyhitMain(inout RayPayload payload, in Attributes attr)
{
    Triangle triangle;
    const float3 barycentricsCoords = float3(1.0f - attr.barycentrics.x - attr.barycentrics.y, 
        attr.barycentrics.x, attr.barycentrics.y);
    
    RTMeshData meshData = commonDataPtr[InstanceIndex()];
    
    const uint primitiveIdx = PrimitiveIndex() * 3 + meshData.indexBufferOffset;
    for (int i = 0; i < 3; ++i)
    {
        const uint index = indexDataPtr[primitiveIdx + i];
        triangle.uv[i] = vertexDataPtr[meshData.vertexBufferOffset + index].UV;
    }
    
    triangle.uv[0].y = -triangle.uv[0].y;
    triangle.uv[1].y = -triangle.uv[1].y;
    triangle.uv[2].y = -triangle.uv[2].y;
    
    const float2 uv = barycentricsCoords.x * triangle.uv[0] 
                    + barycentricsCoords.y * triangle.uv[1] 
                    + barycentricsCoords.z * triangle.uv[2];
    
    const float2 dUvX = GetScreenDerivativeX(uv);
    const float2 dUvY = GetScreenDerivativeY(uv);
    const float2 screenExtent = viewDataPtr->extent;
    
    
    float4 albedoColor = float4(meshData.materialDesc.baseColorFactor, 1.0);
    if (meshData.materialDesc.albedoID > 0)
    {
        albedoColor = textures[meshData.materialDesc.albedoID].SampleGrad(uv, dUvX / screenExtent.x, dUvY / screenExtent.y);
    }
    if (albedoColor.w < meshData.alphaCutoff)
    {
        IgnoreHit();
    }
}