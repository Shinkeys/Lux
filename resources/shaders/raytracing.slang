

//uniform RaytracingAccelerationStructure sceneBVH;

[shader("raygeneration")]
void RaygenMain()
{
    //float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);
    //
    //uint3 rayIdx = DispatchRaysIndex();
    //float3 origin = float3(float(rayIdx.x), float(rayIdx.y), 1.0f);
    //
    //float3 dir = float3(0.0f, 0.0f, -1.0f);
    //
    //RayDesc ray;
    //ray.Origin = origin;
    //ray.Direction = dir;
    //ray.TMin = 0.001;
    //ray.TMax = 10000.0;
    //TraceRay(sceneBVH, RAY_FLAG_NONE, ~0, 0, 0, 0, ray, );
}

[shader("miss")]
void MissMain()
{
    
}

[shader("closesthit")]
void ClosestMain()
{

}