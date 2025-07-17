import common.common;
import common.camera;

struct Vertex
{
    float3 position;
    float3 normal;
    float3 tangent;
    float2 UV;

    uint materialIndex;
};

struct Material
{
    float3 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;

    uint albedoID;
    uint normalID;
    uint metalRoughnessID;
};


[[vk::push_constant]]
cbuffer PushConstants
{
    Vertex* vertexPtr;
    float4x4* modelPtr; // for now just a model matrix
    Material* materialsPtr;
    ViewData* viewDataPtr;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float4 normals; 
    float2 texCoord;
    uint materialIdx;
    float4 worldPos;
    float3x3 TBN;
};

[shader("vertex")]
VertexOutput VertexMain(uint vertexIndex: SV_VertexID)
{
    VertexOutput output = (VertexOutput)0;

    uint index = vertexIndex;
    Vertex vertex = vertexPtr[index];

    float3 T = normalize(mul(*modelPtr, float4(vertex.tangent, 0.0)).xyz);
    float3 N = normalize(mul(*modelPtr, float4(vertex.normal, 0.0)).xyz);
    // Gram-Schmidt process to make vectors orthogonal back
    T = normalize(T - dot(T, N) * N);
    float3 B = normalize(cross(T, N));
    float3x3 TBN = float3x3(T, B, N);
    TBN = transpose(TBN);  // important in slang
    output.TBN = TBN;

    output.position = mul(mul(viewDataPtr.viewProj, *modelPtr), float4(vertex.position, 1.0));

    output.worldPos = mul(*modelPtr, float4(vertex.position, 1.0));

    output.normals =  normalize(float4(mul(TBN, float3(vertex.normal)), 1.0));

    output.texCoord = vertex.UV;
    output.materialIdx = vertex.materialIndex;

    return output;
}

struct FragmentOutput 
{
    [[vk::location(0)]] float4 outWorldPos : SV_Target0;
    [[vk::location(1)]] float4 outNormals : SV_Target1;
    [[vk::location(2)]] float4 outAlbedo : SV_Target2;
    [[vk::location(3)]] float4 outMetallicRoughness : SV_Target3;
};

[vk::binding(0, 0)]
public Sampler2D textures[];


[shader("fragment")]
FragmentOutput FragmentMain(VertexOutput input)
{
    FragmentOutput output = (FragmentOutput)0;

    Material material = materialsPtr[input.materialIdx];

    float2 UV = input.texCoord;
    UV.y = -UV.y;

    output.outWorldPos = input.worldPos;
    output.outNormals  = input.normals;
    if (material.normalID > 0)
    {
        output.outNormals = textures[material.normalID].Sample(UV);
        output.outNormals = float4(normalize(mul(input.TBN, output.outNormals.xyz)), 1.0);
    }
                          // base color is vec3 due to the renderdoc display bug, for now it's totally fine to store it like that
    float4 albedoColor = float4(material.baseColorFactor, 1.0);
    if (material.albedoID > 0)
    {
        albedoColor = textures[material.albedoID].Sample(UV);
    }

    float4 metallicRoughnessColor = float4(0.5, 0.5, 0.5, 1.0);
    if (material.metalRoughnessID > 0)
    {
        metallicRoughnessColor = textures[material.metalRoughnessID].Sample(UV);
        // Apply factors directly in the g buffer pass, so just need to work with the textures directly in main shading pass
        metallicRoughnessColor.b *= material.metallicFactor;
        metallicRoughnessColor.g *= material.roughnessFactor;
    }

    output.outAlbedo = albedoColor;
    output.outMetallicRoughness = metallicRoughnessColor;

    return output;
}
