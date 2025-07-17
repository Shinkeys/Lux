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

float3 CalculateLight(PointLight light, float3 albedo, float3 metallicRoughness, float3 normal, float3 fragPos)
{
    float attenuation = AttenuatePointLight(light.position, fragPos, light.intensity, light.radius);

    float3 wi = normalize(light.position - fragPos); // wi is basically a current light direction incoming to the surf.
    float cosTheta = max(dot(normal, wi), 0.0);
    float3 L = light.color * attenuation * light.intensity; // L is basically a radiance of the rendering equation, cosTheta is the angle between
    float3 V = normalize(viewDataPtr.position - fragPos); // view vector(WORLD SPACE)
    // incoming ray and the surface normal
    float3 halfway = normalize(V + wi);

    float metallic = metallicRoughness.b;
    float roughness = metallicRoughness.g;

    // Fresnel
    float3 F0 = float3(0.04);
    F0 = lerp(F0, albedo, metallic); // interpolate for non-metallic surfaces

    float NDF = DistributionGGX(halfway, normal, roughness);
    float G   = GeometrySmith(normal, V, wi, roughness);
    float3 F  = FresnelSchlick(F0, max(dot(halfway, V), 0.0));

    float3 DFG = NDF * G * F;
    float cookTorranceDenom = 4 * max(dot(normal, V), 0.0) * max(dot(normal, wi), 0.0) + 0.0001; // prevent zero division
    float3 specular = DFG / cookTorranceDenom;

    float3 kS = F; // Fresnel is just a specular
    float3 kD = float3(1.0) - kS;

    kD *= 1.0 - metallic; // if metallic - no diffuse

    float NdotL = max(dot(normal, wi), 0.0);

    return (kD * albedo / Pi + specular) * L * NdotL;
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

        Lo += CalculateLight(pointLight, albedoColor, metallicRoughnessColor, normals, positions);
    }

    float3 ambient = float3(0.001) * albedoColor;
    float3 color = ambient + Lo;
    color = color / (color + float3(1.0));
    color = pow(color, float3(1.0 / 2.2));

    output.color = float4(color, 1.0);

    return output;
}
