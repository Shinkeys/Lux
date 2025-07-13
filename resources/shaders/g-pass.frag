#version 460 core
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_debug_printf : enable


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

layout(scalar, buffer_reference, buffer_reference_align = 4) buffer EntityUniformBuffer
{
	mat4 model;
};

struct Material
{
	vec3  baseColorFactor;
	float metallicFactor;
	float roughnessFactor;

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
	Vertices ptr;
	EntityUniformBuffer uniformPtr;
	Materials materialsPtr;
	ViewDataBuffer viewDataPtr;
};

// IN
layout(location = 0) in vec4 inWorldPos;
layout(location = 1) in vec4 inNormals;
layout(location = 2) in vec2 inUV;
layout(location = 3) in flat uint inMaterialIndex;
layout(location = 4) in mat3 inTBN;




// OUT
layout(location = 0) out vec4 outWorldPos;            // in deferred basically texture 1
layout(location = 1) out vec4 outNormals;			  // in deferred basically texture 2
layout(location = 2) out vec4 outAlbedo;			  // in deferred basically texture 3
layout(location = 3) out vec4 outMetallicRoughness;	  // in deferred basically texture 4
													

layout(binding = 0) uniform sampler2D textures[];


void main()
{
	Material material = materialsPtr.materials[inMaterialIndex];


	vec2 UV = inUV;
	UV.y = -UV.y;

	outWorldPos = inWorldPos;
	outNormals = inNormals;
	if(material.normalID > 0)
	{
		outNormals = texture(textures[material.normalID], UV);
		outNormals = vec4(normalize(inTBN * vec3(outNormals)), 1.0);
	}


	vec3 albedoColor = vec3(material.baseColorFactor);
	if(material.albedoID > 0)
	{
		albedoColor = texture(textures[material.albedoID],  UV).xyz;
	}
	

	vec3 metallicRoughnessColor = vec3(0.5, 0.5, 0.5);
	if(material.metalRoughnessID > 0)
	{
		metallicRoughnessColor = texture(textures[material.metalRoughnessID],  UV).xyz;
		// Apply factors directly in the g buffer pass, so just need to work with the textures directly in main shading pass
		metallicRoughnessColor.b *= material.metallicFactor;
		metallicRoughnessColor.g *= material.roughnessFactor;
	}

	outAlbedo = vec4(albedoColor, 1.0);
	outMetallicRoughness = vec4(metallicRoughnessColor, 1.0);
}