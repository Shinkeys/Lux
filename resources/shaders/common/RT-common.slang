import mesh_common;

public struct RTMeshData
{
    public Material materialDesc;
    public uint vertexBufferOffset;
    public uint indexBufferOffset;
    public float alphaCutoff;
};

public struct Triangle
{
    public Array<float3, 3> pos;
    public Array<float3, 3> tangent;
    public Array<float2, 3> uv;
}

public float2 GetScreenDerivativeX(float2 position, float step = 1.0f /* 1 pixel */)
{
    return position + float2(step, 0.0f) - position / step;
}

public float2 GetScreenDerivativeY(float2 position, float step = 1.0f /* 1 pixel */)
{
    return position + float2(0.0f, step) - position / step;
}

public struct [raypayload] RayPayload
{
    public float4 color : read(caller) : write(caller, closesthit, miss);
    public float distance : read(caller) : write(closesthit);
    public RayDesc ray : read(caller) : write(caller, closesthit);
    public float seed : read(closesthit) : write(caller, closesthit);
    public bool hitLight : read(caller) : write(closesthit);
};

// A function to check if the light hit is performed.
// True: no obstacles on the way - no closest hit shader was called
// Useful to check if light hit was performed by setting TMax to val - -0.001f
public bool NoObstaclesRay(in RaytracingAccelerationStructure tlas, RayDesc rd, uint mask)
{
    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> rq;

    rq.TraceRayInline(tlas, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, mask, rd);

    rq.Proceed(); // Important to traverse AS

    return (rq.CommittedStatus() == COMMITTED_NOTHING);
}
