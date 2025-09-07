uniform RaytracingAccelerationStructure sceneBVH;
uniform RWTexture2D resultTexture;

struct [raypayload] RayPayload
{
    float4 color : read(caller) : write(caller, closesthit, miss);
};

[shader("raygeneration")]
void RaygenMain()
{    
    uint3 rayIdx = DispatchRaysIndex();

    float2 uv = (float2(rayIdx.xy) + 0.5) / float2(DispatchRaysDimensions().xy);
    uv.x = (2.0f * rayIdx.x) / DispatchRaysDimensions().x - 1.0f;
    uv.y = 1.0f - (2.0f * rayIdx.y) / DispatchRaysDimensions().y;
    float3 origin = float3(uv, -1.0f);
    
    float3 dir = float3(0.0f, 0.0f, 1.0f);
    
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = dir;
    ray.TMin = 0.001;
    ray.TMax = 10000.0;
    RayPayload payload = { float4(0, 0, 0, 0) };
    TraceRay(sceneBVH, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, ~0, 0, 0, 0, ray, payload);

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