#pragma once
#include "../util/util.h"
#include "../scene/component.h"

#include <glm/glm.hpp>

struct Geometry
{
	glm::vec3 position{ glm::vec3(0.0f) };
	glm::vec3 normal{ glm::vec3(0.0f) };
	glm::vec3 tangent{ glm::vec3(0.0f) };
	glm::vec2 UV{ glm::vec2(0.0f) };
};

struct GeometryDescription
{
	const Geometry* geometryPtr{ nullptr };
	u32 geometryCount{ 0 };
	const u32* indicesPtr{ nullptr };
	u32 indexCount{ 0 };
};

struct Mesh
{
	TranslationComponent* transform;
	GeometryDescription geometryDesc;
	//ptr* materialPtr;
};

struct LoadedMesh
{
	std::vector<Geometry> geometry;
	std::vector<u32> indices;
};

struct LoadedTextures
{
	/*std::vector<Texture*/
};

struct LoadedGLTF
{
	bool isLoaded{ false };


	std::vector<LoadedMesh> meshes;
	std::vector<LoadedTextures> textures;
};
