#version 460 core
#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require

// Y is reverted in vulkan
// Y is reverted in vulkan
// Y is reverted in vulkan

struct Geometry
{
	vec3 position;
	vec3 normals;
	vec3 tangents;
	vec2 UVs;
};

layout(scalar, buffer_reference, buffer_reference_align = 8) buffer Vertices
{
	Geometry geometry[];
};

layout(scalar, buffer_reference, buffer_reference_align = 8) buffer UniformBuffer
{
	mat4 model;
	mat4 view;
	mat4 proj;
};


layout(push_constant) uniform buffersPtr
{
	Vertices ptr;
	UniformBuffer uniformPtr;
};



void main()
{
	uint index = gl_VertexIndex;
	vec3 position = ptr.geometry[index].position;

	gl_Position = uniformPtr.proj * uniformPtr.view * uniformPtr.model * vec4(position.xyz, 1.0);
}