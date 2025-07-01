#pragma once
#include "../util/util.h"
#include "asset_types.h"

// forward decl
namespace fastgltf
{
	class Asset;
	class Mesh;
	class Image;
}

class ModelImporter
{
private:
	using EntityIndex = u32;

	bool LoadMeshes(const fastgltf::Asset& asset, LoadedGLTF& gltfData);
	void StoreTextureData(const fastgltf::Image& image, TexturesData& InTexture);
public:
	LoadedGLTF LoadGltf(const fs::path& path);

};