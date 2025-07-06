#version 460 core
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : enable



layout (location = 0) out vec4 FragColor;

struct PointLight
{
	vec3 position;
	vec3 color;
	float intenstity;
	float radius;
};

layout(scalar, buffer_reference, buffer_reference_align = 4) buffer PointLights
{
	PointLight pointLights[];
};

layout(push_constant) uniform pushConst
{
	PointLights lightsPtr;

	uint positionTexIndex;
	uint normalsTexIndex;
	uint albedoTexIndex;
	uint metallicRoughnessTexIndex;

	uint pointLightsCount;
};

layout(binding = 0) uniform sampler2D textures[];

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inUV;


//
float Square(float x)
{
    return x * x;
}

float AttenuatePointLight(vec3 lightPos, vec3 fragPos, float radius)
{
    const float distance = length(lightPos - fragPos);
    const float decaySpeed = 1.0;
    const float maxIntensity = 2.5; // basically represents 'start' point of the light brightness
    const float s = distance / radius;
    const float sqrS = Square(s);
    // check if distance < radius, otherwise would get wrong lightness
    // values at larger distances
    if(s >= 1.0)
        return 0.0;

    return maxIntensity * Square(1 - sqrS) / (1 + decaySpeed * sqrS);
}


vec3 CalculateLight(vec3 position, vec3 normal)
{
	float attenuation = AttenuatePointLight(lightsPtr.pointLights[0].position, position, lightsPtr.pointLights[0].radius);

	vec3 lightColor = lightsPtr.pointLights[0].color;
	vec3 ambientStr = 0.10 * lightColor;

	vec3 lightDirection =  lightsPtr.pointLights[0].position - position;
	float diffuseInt = max(dot(normal, lightDirection), 0.0);
	vec3 diffuse = diffuseInt * lightColor * attenuation;



	return ambientStr + diffuse;
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

	vec3 resultColor = albedoColor * CalculateLight(positions, normals);

	FragColor = vec4(resultColor, 1.0);
}