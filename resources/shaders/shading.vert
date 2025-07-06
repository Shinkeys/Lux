#version 460 core
#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_debug_printf : enable


// Y is reverted in vulkan
// Y is reverted in vulkan
// Y is reverted in vulkan

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

const vec3 vertices[] = 
{
  vec3(-1.0f, -1.0f, 0.0f),
  vec3(1.0f, -1.0f, 0.0f),
  vec3(1.0f, 1.0f, 0.0f),
  vec3(1.0f, 1.0f, 0.0f),
  vec3(-1.0f, 1.0f, 0.0f),
  vec3(-1.0f, -1.0f, 0.0f)
};

const vec2 texCoords[] = 
{
	vec2(0.0f, 1.0f),
    vec2(1.0f, 1.0f),
    vec2(1.0f, 0.0f),
    vec2(1.0f, 0.0f),
    vec2(0.0f, 0.0f),
    vec2(0.0f, 1.0f)
};


layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec2 outUV;
			
void main()
{
	outPosition = vec4(vertices[gl_VertexIndex], 1.0);
	gl_Position = outPosition;

	outUV = texCoords[gl_VertexIndex];
}