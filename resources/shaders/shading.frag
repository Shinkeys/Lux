#version 460 core
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_debug_printf : enable


layout (location = 0) out vec4 FragColor;


struct Vertex
{
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec2 UV;

	uint materialIndex;
};

layout(scalar, buffer_reference, buffer_reference_align = 4) buffer Vertices
{
	Vertex vertex[];
};

layout(scalar, buffer_reference, buffer_reference_align = 4) buffer UniformBuffer
{
	mat4 model;
	mat4 view;
	mat4 proj;
};

struct Material
{
	uint albedoID;
	uint normalID;
	uint metalRoughnessID;
};

layout(scalar, buffer_reference, buffer_reference_align = 4) buffer Materials
{
	Material materials[];
};


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


layout(binding = 0) uniform sampler2D textures[];


layout(location = 0) in vec2 inUV;
layout(location = 1) in flat uint inMaterialIndex;
layout(location = 2) in vec3 inFragPos;




layout(push_constant) uniform buffersPtr
{
	Vertices ptr;
	UniformBuffer uniformPtr;
	Materials materialsPtr;
	PointLights lightsPtr;
	uint pointLightsCount;
};


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


vec3 CalculateLight(vec3 normal)
{
	float attenuation = AttenuatePointLight(lightsPtr.pointLights[0].position, inFragPos, lightsPtr.pointLights[0].radius);

	vec3 lightColor = lightsPtr.pointLights[0].color;
	vec3 ambientStr = 0.10 * lightColor;

	vec3 lightDirection =  lightsPtr.pointLights[0].position - inFragPos;
	float diffuseInt = max(dot(normal, lightDirection), 0.0);
	vec3 diffuse = diffuseInt * lightColor * attenuation;



	return ambientStr + diffuse;
}


void main()
{
	Material material = materialsPtr.materials[inMaterialIndex];

	vec2 UV = inUV;
	UV.y = -UV.y;

	vec3 albedoColor = vec3(0.5, 0.5, 0.5);
	if(material.albedoID > 0)
	{
		albedoColor = texture(textures[material.albedoID],  UV).xyz;
	}

	vec3 normal = vec3(0.0, 0.0, 0.0);
	if(material.normalID > 0)
	{
		normal = texture(textures[material.normalID], UV).xyz;
	}
	
	vec3 metallicRoughnessColor = vec3(0.5, 0.5, 0.5);
	if(material.metalRoughnessID > 0)
	{
		metallicRoughnessColor = texture(textures[material.metalRoughnessID],  UV).xyz;
	}
	
	vec3 finalColor = albedoColor * CalculateLight(normal);
	FragColor = vec4(finalColor, 1.0);
}