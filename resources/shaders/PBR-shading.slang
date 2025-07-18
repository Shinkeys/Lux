import common.camera;
import common.common;
import common.lights;
import common.PBR_common;


[[vk::push_constant]]
cbuffer PushConstants
{ 
    PointLight *lightsPtr;
    int *lightIndicesPtr;
    ViewData *viewDataPtr;

    uint positionTexIndex;
    uint normalsTexIndex;
    uint albedoTexIndex;
    uint metallicRoughnessTexIndex;

    uint pointLightsCount;
    uint tileSize;
};

static const Array<float3, 6> vertices = 
{
    float3(-1.0f, -1.0f, 0.0f),
    float3(1.0f, -1.0f, 0.0f),
    float3(1.0f, 1.0f, 0.0f),
    float3(1.0f, 1.0f, 0.0f),
    float3(-1.0f, 1.0f, 0.0f),
    float3(-1.0f, -1.0f, 0.0f)
};

static const Array<float2, 6> texCoords =
{
	float2(0.0f, 1.0f),
    float2(1.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 0.0f),
    float2(0.0f, 1.0f)
};

struct VertexInput
{
    uint vertexIndex : SV_VertexID;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float2 texCoord;
};

[shader("vertex")]
VertexOutput VertexMain(VertexInput input)
{
    VertexOutput output = (VertexOutput)0;
    output.position = float4(vertices[input.vertexIndex], 1.0);
    output.texCoord = float2(texCoords[input.vertexIndex]);

    return output;
}

[vk::binding(0, 0)]
public Sampler2D textures[];
[vk::binding(1, 0)]
public RWTexture2D<uint2> lightsGrid;

struct FragmentOutput 
{
    float4 color : SV_TARGET0;
};

[shader("fragment")]
FragmentOutput FragmentMain(VertexOutput input)
{
    FragmentOutput output = (FragmentOutput)0;

    float3 positions = float3(0.0);
    if (positionTexIndex > 0)
    {
        positions = textures[positionTexIndex].Sample(input.texCoord).xyz;
    }

    float3 albedoColor = float3(0.5);
    if (albedoTexIndex > 0)
    {
        albedoColor = textures[albedoTexIndex].Sample(input.texCoord).xyz;
    }

    float3 normals = float3(0.0);
    if (normalsTexIndex > 0)
    {
        normals = textures[normalsTexIndex].Sample(input.texCoord).xyz;
    }

    float3 metallicRoughnessColor = float3(0.0);
    if (metallicRoughnessTexIndex > 0)
    {
        metallicRoughnessColor = textures[metallicRoughnessTexIndex].Sample(input.texCoord).xyz;
    }

    int3 fragCoord = int3(input.position.xyz);

    uint2 lightsDataInTile = lightsGrid.Load(int2(fragCoord.x / tileSize, fragCoord.y / tileSize));
    uint startIndex = lightsDataInTile.x;
    uint lightsCount = lightsDataInTile.y;  

    float3 lightingResult = albedoColor * 0.1;

    float3 Lo = float3(0.0);
    for (uint i = 0; i < lightsCount; ++i)
    {
        uint lightIndex = lightIndicesPtr[i + startIndex];
        PointLight pointLight = lightsPtr[lightIndex];

        Lo += CalculateLight(pointLight, albedoColor, metallicRoughnessColor, normals, viewDataPtr.position, positions);
    }

    float3 ambient = float3(0.001) * albedoColor;
    float3 color = ambient + Lo;
    color = color / (color + float3(1.0));
    color = pow(color, float3(1.0 / 2.2));

    output.color = float4(color, 1.0);

    return output;
}
