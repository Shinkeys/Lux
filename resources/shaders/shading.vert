#version 460 core
#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_debug_printf : enable


// Y is reverted in vulkan
// Y is reverted in vulkan
// Y is reverted in vulkan

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


layout(push_constant) uniform buffersPtr
{
	Vertices ptr;
	UniformBuffer uniformPtr;
	Materials materialsPtr;
};


layout(location = 0) out vec2 outUV;
layout(location = 1) out flat uint outMaterialIndex;

void main()
{ 
	uint index = gl_VertexIndex;
	vec3 position = ptr.vertex[index].position;



	gl_Position = uniformPtr.proj * uniformPtr.view * uniformPtr.model * vec4(position.xyz, 1.0);

	outUV = ptr.vertex[index].UV;
	outMaterialIndex = ptr.vertex[index].materialIndex;
}