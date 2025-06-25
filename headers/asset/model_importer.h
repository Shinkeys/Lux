#pragma once
#include "../util/util.h"
#include "asset_types.h"

// forward decl
namespace fastgltf
{
	class Asset;
	class Mesh;
}

class ModelImporter
{
private:
	using EntityIndex = u32;

	bool LoadMeshes(const fastgltf::Asset& asset, std::vector<fastgltf::Mesh>& meshes, LoadedGLTF& gltfData);


public:
	LoadedGLTF LoadGltf(const fs::path& path);

};