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

layout(binding = 0) uniform sampler2D textures[];


layout(location = 0) in vec2 inUV;
layout(location = 1) in flat uint inMaterialIndex;


layout(push_constant) uniform buffersPtr
{
	Vertices ptr;
	UniformBuffer uniformPtr;
	Materials materialsPtr;
};

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
	
	vec3 finalColor = albedoColor + metallicRoughnessColor;
	FragColor = vec4(albedoColor, 1.0);
}