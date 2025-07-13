#version 460 core
#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
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


layout(location = 0) out vec4 outWorldPos;
layout(location = 1) out vec4 outNormals;
layout(location = 2) out vec2 outUV;
layout(location = 3) out flat uint outMaterialIndex;
layout(location = 4) out mat3 outTBN;


void main()
{
	uint index = gl_VertexIndex;
	Vertex vertex = ptr.vertex[index];


	vec3 T = normalize(vec3(uniformPtr.model * vec4(vertex.tangent, 0.0)));
	vec3 N = normalize(vec3(uniformPtr.model * vec4(vertex.normal, 0.0)));
	// Gram-Schmidt process to make vectors orthogonal back
	T = normalize(T - dot(T, N) * N);
	vec3 B = normalize(cross(T, N));
	mat3 TBN = mat3(T,B,N);

	outTBN = TBN;

	gl_Position = viewDataPtr.viewData.proj * viewDataPtr.viewData.view * uniformPtr.model * vec4(vertex.position, 1.0);

	outWorldPos = uniformPtr.model * vec4(vertex.position, 1.0);

	outNormals = vec4(normalize(TBN * vec3(vertex.normal)), 1.0);

	outUV = vertex.UV;
	outMaterialIndex = vertex.materialIndex;
}