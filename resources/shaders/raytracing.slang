import common.camera;
import common.RT_common;
import common.mesh_common;

[vk::binding(0, 0)]
public RaytracingAccelerationStructure sceneBVH;
[vk::binding(1, 0)]
public RWTexture2D resultTexture;
[vk::binding(2, 0)]
public Sampler2D textures[];

struct [raypayload] RayPayload
{
    float4 color : read(caller) : write(caller, closesthit, miss);
};

struct Attributes
{
    float2 barycentrics : SV_BaryCentrics;
    float2 anisotropy : ANISOTROPY;
}

struct Triangle
{
    Array<float2, 3> uv;
}

[[vk::push_constant]]
cbuffer PushConstants
{
    ViewData     *viewDataPtr;
    Vertex       *vertexDataPtr;
    uint         *indexDataPtr;
    RTMeshData   *commonDataPtr;

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

    RayDesc ray;
    ray.Origin = originWorld;
    ray.Direction = dirWorld;
    ray.TMin = 0.001;
    ray.TMax = 10000.0;
    RayPayload payload = { float4(0, 0, 0, 0) };
    TraceRay(sceneBVH, RAY_FLAG_NONE, ~0, 0, 0, 0, ray, payload);

    resultTexture.Store(rayIdx.xy, payload.color); 
 
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

    const int albedoID = meshData.materialDesc.albedoID;

    const float2 dUvX  = GetScreenDerivativeX(uv);
    const float2 dUvY  = GetScreenDerivativeY(uv);
    const float2 screenExtent = viewDataPtr->extent;

    payload.color = textures[albedoID].SampleGrad(uv, dUvX / screenExtent.x, dUvY / screenExtent.y);
}