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
    float3 origin = float3(float(rayIdx.x), float(rayIdx.y), 1.0f);
    
    float3 dir = float3(0.0f, 0.0f, -1.0f);
    
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = dir;
    ray.TMin = 0.001;
    ray.TMax = 10000.0;
    RayPayload payload = { float4(0, 0, 0, 0) };
    TraceRay(sceneBVH, RAY_FLAG_NONE, ~0, 0, 0, 0, ray, payload);

    resultTexture[rayIdx.xy] = payload.color;
 
}

[shader("miss")]
void MissMain(inout RayPayload payload)
{
    payload.color = float4(1.0f, 0.0f, 0.0f, 1.0f);
}

[shader("closesthit")]
void ClosestMain(inout RayPayload payload)
{
    for (uint i = 0; i < 3; ++i)
    {
        float3 vertex = HitTriangleVertexPosition(i);

        payload.color = float4(1.0f, 0.0f, 0.0f, 1.0f);
    }
}

[shader("vertex")]
float4 VertexMain(uint idx : SV_VertexID) : SV_Position
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}

[shader("fragment")]
float4 FragmentMain(float4 sv_position: SV_Position) : SV_Target
{
    return resultTexture.Load(uint2(sv_position.xy));
}