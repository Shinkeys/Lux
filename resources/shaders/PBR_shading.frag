#version 460 core
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_debug_printf : enable

const float Pi = 3.14159265359;


layout (location = 0) out vec4 FragColor;

struct PointLight
{
	vec3 position;
	vec3 color;
	float intensity;
	float radius;
};

layout(scalar, buffer_reference, buffer_reference_align = 4) buffer PointLights
{
	PointLight pointLights[];
};

layout(scalar, buffer_reference, buffer_reference_align = 4) buffer LightIndices
{
	int lightIndices[];
};

struct ViewData
{
	mat4 view;
	mat4 proj;
	mat4 viewProj;
	mat4 inverseProjection;
	vec3 position;

	ivec2 extent;
	float nearPlane;
	float farPlane;
};

layout(scalar, buffer_reference, buffer_reference_align = 4) buffer ViewDataBuffer
{
	ViewData viewData;
};

layout(push_constant) uniform pushConst
{
	PointLights  	lightsPtr;
	LightIndices 	lightIndicesPtr;
	ViewDataBuffer  viewDataPtr;

	uint positionTexIndex;
	uint normalsTexIndex;
	uint albedoTexIndex;
	uint metallicRoughnessTexIndex;

	uint pointLightsCount;
	uint tileSize;
};

layout(binding = 0) uniform sampler2D textures[];

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inUV;


layout(rg32ui, binding = 1) uniform readonly uimage2D lightsGrid;


// NEEDED PBR CALCULATIONS
vec3 FresnelSchlick(vec3 F0, float cosTheta) // angle between halfway and viewdir
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0); // clamp is needed just in case if some floating point error would happen
}

float DistributionGGX(vec3 halfway, vec3 normal, float roughness)
{
	float a = roughness * roughness; 
	float aSq = a * a;
	float NdotH = max(dot(normal, halfway), 0.0);
	float NdotHSq = NdotH * NdotH;

	float denom = NdotHSq * (aSq - 1.0) + 1.0;
	denom = Pi * denom * denom;

	return aSq / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float kDirect = (r * r) / 8.0;

	float denom = NdotV * (1.0 - kDirect) + kDirect;

	return NdotV / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 wi, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, wi), 0.0); // wi - light's direction

	float ggx1 = GeometrySchlickGGX(NdotV, roughness);
	float ggx2 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}



//
float Square(float x)
{
    return x * x;
}

float AttenuatePointLight(vec3 lightPos, vec3 fragPos, float intensity, float radius)
{
    const float distance = length(lightPos - fragPos);
    const float decaySpeed = 1.0;
    const float maxIntensity = intensity; // basically represents 'start' point of the light brightness
    const float s = distance / radius;
    const float sqrS = Square(s);
    // check if distance < radius, otherwise would get wrong lightness
    // values at larger distances
    if(s >= 1.0)
        return 0.0;

    return maxIntensity * Square(1 - sqrS) / (1 + decaySpeed * sqrS);
}


vec3 CalculateLight(PointLight light, vec3 albedo, vec3 metallicRoughness, vec3 normal, vec3 fragPos)
{
	float attenuation = AttenuatePointLight(light.position, fragPos, light.intensity, light.radius);

	vec3 wi = normalize(light.position - fragPos); // wi is basically a current light direction incoming to the surf.
	float cosTheta = max(dot(normal, wi), 0.0);
	vec3 L = light.color * attenuation * light.intensity; // L is basically a radiance of the rendering equation, cosTheta is the angle between
	vec3 V = normalize(viewDataPtr.viewData.position - fragPos); // view vector(WORLD SPACE)
	// incoming ray and the surface normal
	vec3 halfway = normalize(V + wi);

	float metallic  = metallicRoughness.b;
	float roughness = metallicRoughness.g;

	// Fresnel
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic); // interpolate for non-metallic surfaces

	float NDF = DistributionGGX(halfway, normal, roughness);
	float G   = GeometrySmith(normal, V, wi, roughness);
	vec3 F  = FresnelSchlick(F0, max(dot(halfway, V), 0.0));

	vec3 DFG = NDF * G * F;
	float cookTorranceDenom = 4 * max(dot(normal, V), 0.0) * max(dot(normal, wi), 0.0) + 0.0001; // prevent zero division
	vec3 specular = DFG / cookTorranceDenom;

	vec3 kS = F; // Fresnel is just a specular
	vec3 kD = vec3(1.0) - kS;

	kD *= 1.0 - metallic; // if metallic - no diffuse

	float NdotL = max(dot(normal, wi), 0.0);

	return (kD * albedo / Pi + specular) * L * NdotL;	
}

void main()
{
	vec3 positions = vec3(0.0);
	if(positionTexIndex > 0)
	{
		positions = texture(textures[positionTexIndex], inUV).xyz;
	}

	vec3 albedoColor = vec3(0.5);
	if(albedoTexIndex > 0)
	{
		albedoColor = texture(textures[albedoTexIndex], inUV).xyz;
	}

	vec3 normals = vec3(0.0);
	if(normalsTexIndex > 0)
	{
		normals = texture(textures[normalsTexIndex], inUV).xyz;
	}

	vec3 metallicRoughnessColor = vec3(0.0);
	if(metallicRoughnessTexIndex > 0)
	{
		metallicRoughnessColor = texture(textures[metallicRoughnessTexIndex], inUV).xyz;
	}

	uvec4 lightsDataInTile = imageLoad(lightsGrid, ivec2(gl_FragCoord.xy / tileSize));
	uint  startIndex  = lightsDataInTile.x;
	uint  lightsCount = lightsDataInTile.y;

	vec3 lightingResult = albedoColor * 0.1;
	
	vec3 Lo = vec3(0.0);
	for(uint i = 0; i < lightsCount; ++i)
	{
		uint lightIndex = lightIndicesPtr.lightIndices[i + startIndex];
		PointLight pointLight = lightsPtr.pointLights[lightIndex];
		
		Lo += CalculateLight(pointLight, albedoColor, metallicRoughnessColor, normals, positions);
	}

	vec3 ambient = vec3(0.001) * albedoColor;
	vec3 color = ambient + Lo;
	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0/2.2)); 

	FragColor = vec4(color, 1.0);
}