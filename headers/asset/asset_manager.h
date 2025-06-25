#pragma once

#include "../util/util.h"
#include "asset_storage.h"
#include "model_importer.h"


class AssetManager
{
private:
	AssetStorage _storage;
	ModelImporter _importer;


	using AssetID = u32;
	AssetID _currentAvailableIndex{ 1 }; // 0 is reserved
	std::unordered_map<AssetID, GeometryDescription> _geometryDescription;

	fs::path ConvertToPath(const fs::path& folder);
	fs::path FindGLTFByPath(const fs::path& path);
public:
	size_t GetAllSceneSize();

	void UpdateDataPointers();
	const GeometryDescription* GetGeometryDesc(AssetID id) const;

	/**
	* @brief Write models FOLDER to load the file from it. There's should be files only for one model
	*/
	AssetID TryToLoadAndStoreMesh(const fs::path& folder);
};