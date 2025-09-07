import common.camera;

uniform RaytracingAccelerationStructure sceneBVH;
uniform RWTexture2D resultTexture;

struct [raypayload] RayPayload
{
    float4 color : read(caller) : write(caller, closesthit, miss);
};

[[vk::push_constant]]
cbuffer PushConstants
{
    ViewData *viewDataPtr;
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
void ClosestMain(inout RayPayload payload)
{
    payload.color = float4(1.0f, 0.0f, 0.0f, 1.0f);
}