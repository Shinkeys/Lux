import common.common;
import common.g_pass;
import common.camera;
import common.mesh_common;

[[vk::push_constant]]
cbuffer PushConstants
{
    Vertex *vertexPtr;
    CommonMeshData *commonMeshDataPtr;
    ViewData *viewDataPtr;

    uint baseDrawOffset;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float4 normals;
    float2 texCoord;
    float4 worldPos;
    float3x3 TBN;
    Material material;
};

[shader("vertex")]
VertexOutput VertexMain(uint vertexIndex: SV_VertexID, uint indirectIndex: SV_DrawIndex)
{

    VertexOutput output = (VertexOutput)0;

    // indirect index is just a current draw index of indirect buffer, base draw offset is needed to adjust
    // between different types of indirect buffers

    Material material = commonMeshDataPtr[indirectIndex].materialsDesc;
    Transform transform = commonMeshDataPtr[indirectIndex].transformDesc;

    Vertex vertex = vertexPtr[vertexIndex];

    float3 T = normalize(mul(transform.model, float4(vertex.tangent, 0.0)).xyz);
    float3 N = normalize(mul(transform.model, float4(vertex.normal, 0.0)).xyz);

    // Gram-Schmidt process to make vectors orthogonal back
    T = normalize(T - dot(T, N) * N);
    float3 B = normalize(cross(T, N));
    float3x3 TBN = float3x3(T, B, N);
    TBN = transpose(TBN); // important in slang
    output.TBN = TBN;

    output.position = mul(mul(viewDataPtr.viewProj, transform.model), float4(vertex.position, 1.0));

    output.worldPos = mul(transform.model, float4(vertex.position, 1.0));

    output.normals = normalize(float4(mul(TBN, float3(vertex.normal)), 1.0));

    output.texCoord = vertex.UV;

    output.material = material;

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

    Material material = input.material;

    float2 UV = input.texCoord;
    UV.y = -UV.y;

    output.outWorldPos = input.worldPos;
    output.outNormals = input.normals;
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