module camera;

public struct ViewData
{
    public float4x4 view;
    public float4x4 proj;
    public float4x4 viewProj;
    public float4x4 inverseProjection;
    public float3 position;

    public int2 extent;
    public float nearPlane;
    public float farPlane;
};
