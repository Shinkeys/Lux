import camera;
import common;
import lights;
import mesh_common;

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

public struct PathTracingInput
{
    public float randomSeed;
    public uint  accumulatedFrames;
};


[[vk::push_constant]]
public cbuffer PushConstants
{
    public ViewData *viewDataPtr;
    public Vertex *vertexDataPtr;
    public uint *indexDataPtr;
    public RTMeshData *commonDataPtr;
    public PointLight *pointLightsPtr;
    public PathTracingInput *inputPtr;
    public uint maxRecursionDepth;
}


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