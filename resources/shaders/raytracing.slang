import common.camera;
import common.RT_common;
import common.mesh_common;
import common.lights;

[vk::binding(0, 0)]
public RaytracingAccelerationStructure sceneBVH;
[vk::binding(1, 0)]
public RWTexture2D resultTexture;
[vk::binding(2, 0)]
public Sampler2D textures[];

struct [raypayload] RayPayload
{
    float3 dir   : read(caller);
    float4 color : read(caller) : write(caller, closesthit, miss);
    float dot : read(caller) : write(closesthit);
    float3 normal : read(caller) : write(closesthit);
    float reflector : read(caller) : write(closesthit);
};

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
    ray.TMin = 0.001;
    ray.TMax = 10000.0;

    for (uint i = 0; i < maxRecursionDepth; ++i)
    {
        RayPayload payload = { dirWorld, float4(0, 0, 0, 0) };
        TraceRay(sceneBVH, RAY_FLAG_NONE, ~0, 0, 0, 0, ray, payload);

        float4 hitColor = payload.color;

        // ray dir and normal dot is - meaning they're looking in a separate dirs
        if (payload.dot < 0.0f)
        {
            color += 0.5f * hitColor;
            break;
        }
        else if (payload.reflector == 1.0f)
        {
            const float3 hitPos = ray.Origin + ray.Direction + payload.dot;
            ray.Origin = hitPos + payload.normal + 0.001f;
            ray.Direction = reflect(ray.Direction, payload.normal);
        }
        else
        {
            color += 0.5f * hitColor;
            break;
        }
    }


    resultTexture.Store(rayIdx.xy, color); 
}

[shader("miss")]
void MissMain(inout RayPayload payload)
{
    payload.color = float4(0.0f, 0.3f, 0.3f, 1.0f);
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

    const float2 dUvX  = GetScreenDerivativeX(uv);
    const float2 dUvY  = GetScreenDerivativeY(uv);
    const float2 screenExtent = viewDataPtr->extent;

    const float4 normal = textures[normalID].SampleGrad(uv, dUvX / screenExtent.x, dUvY / screenExtent.y);
    const float4 albedo = textures[albedoID].SampleGrad(uv, dUvX / screenExtent.x, dUvY / screenExtent.y);
    payload.color = albedo;
    payload.dot = dot(payload.dir, normal.xyz);
    payload.normal = normal.xyz;
    // if object is fully white we can reflect it
    payload.reflector = (albedo.x == 1.0f && albedo.y == 1.0f && albedo.z == 1.0f) ? 1.0f : 0.0f;
}