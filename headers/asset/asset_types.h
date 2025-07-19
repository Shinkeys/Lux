#pragma once
#include "../util/util.h"
#include "../scene/component.h"

#include <glm/glm.hpp>

struct Vertex
{
	glm::vec3 position{ glm::vec3(0.0f) };
	glm::vec3 normal{ glm::vec3(0.0f) };
	glm::vec3 tangent{ glm::vec3(0.0f) };
	glm::vec2 UV{ glm::vec2(0.0f) };
};

struct VertexDescription
{
	const Vertex* vertexPtr{ nullptr };
	u32 vertexCount{ 0 };
	const u32* indicesPtr{ nullptr };
	u32 indexCount{ 0 };
};

// for materials to differentiate their types
struct AlphaMode
{
	enum class AlphaType : u8
	{
		ALPHA_OPAQUE,
		ALPHA_MASK,
		ALPHA_BLEND,
	};

	AlphaType type{ AlphaType::ALPHA_OPAQUE };

	float alphaCutoff{ 0.0f };

};

struct MaterialTexturesDesc
{
	glm::vec3 baseColorFactor{ glm::vec3(0.0f) };
	float metallicFactor{ 0.0f };
	float roughnessFactor{ 0.0f };

	u32 albedoID{ 0 };
	u32 normalID{ 0 };
	u32 metalRoughnessID{ 0 };

};

struct MaterialDescription
{
	const MaterialTexturesDesc* materialTexturesPtr{ nullptr };
	u32 materialsCount{ 0 };
};

struct SubmeshDescription
{
	VertexDescription vertexDesc{};
	MaterialDescription materialDesc{};
	AlphaMode alphaMode{};
};


struct Mesh
{
	TransformComponent* transform;
	Vertex vertexDesc;
	//ptr* materialPtr;
};

struct LoadedMesh
{
	std::vector<Vertex> vertex;
	std::vector<u32> indices;

	AlphaMode alphaMode{};
};

enum class TextureType : u8
{
	TEXTURE_NONE,
	TEXTURE_ALBEDO,
	TEXTURE_NORMAL,
	TEXTURE_METALLICROUGHNESS,
};

struct TexturesData
{
	using TextureIndex = u32;
	enum class Type
	{
		URI,
		EMBEDDED
	} dataType;

	TextureType textureType = TextureType::TEXTURE_NONE;

	float factor{ 0.0f };
	fs::path path = ""; // if URI
	std::vector<u8> bytes; // if embedded type
};


// Values to store in asset storage about materials to load images later and create a buffer ot textures per material
struct MeshMaterial
{
	std::vector<TexturesData> materialTextures;
	glm::vec3 baseColorFactor{ glm::vec3(0.0f) };
	float metallicFactor{ 0.0f };
	float roughnessFactor{ 0.0f };
	u32 materialIndex{ 0 };
};


struct LoadedGLTF
{
	bool isLoaded{ false };
	std::vector<LoadedMesh> meshes;

	std::vector<MeshMaterial> materials;
};
