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
