import common.common;
import common.camera;

public struct Vertex
{
    public float3 position;
    public float3 normal;
    public float3 tangent;
    public float2 UV;
};

public struct Transform
{
    public float4x4 model;
};

public struct Material
{
    public float3 baseColorFactor;
    public float metallicFactor;
    public float roughnessFactor;
 
    public uint albedoID;
    public uint normalID;
    public uint metalRoughnessID;
};

public struct CommonMeshData
{
    public Material materialsDesc;
    public Transform transformDesc;
   
    public float alphaCutoff;
};
