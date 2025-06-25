#include "../../headers/asset/asset_manager.h"

#include "../../headers/util/helpers.h"
// Purpose: used to create vertex buffer for example.

size_t AssetManager::GetAllSceneSize()
{
	return _storage.GetRawDataSize();
}

// WORKS ONLY WITH GLTF
AssetManager::AssetID AssetManager::TryToLoadAndStoreMesh(const fs::path& folder)
{
	fs::path pathToLoad = ConvertToPath(folder);
	fs::path finalPath = FindGLTFByPath(pathToLoad);

	LoadedGLTF loadedGLTF = _importer.LoadGltf(finalPath);
	if (!loadedGLTF.isLoaded)
	{
		std::cout << "Unable to load model by provided folder: " << folder << '\n';
		return 0;
	}

	StoreGeometryResult result = _storage.StoreGeometry(loadedGLTF, 1);
	if (result.shouldUpdateGeometryPtrs || result.shouldUpdateIndicesPtrs) // update em both at once. doesnt affect performance anyway
		UpdateDataPointers();
	
	assert(result.desc.geometryCount > 0 && result.desc.indexCount > 0 && "Geometry or index count is 0 in TryToLoadAndStoreMesh(). Check the logic");
	assert(result.desc.geometryPtr != nullptr && result.desc.indicesPtr != nullptr && "Geometry ptr or indices ptr is nullptr in TryToLoadAndStoreMesh(). Check the logic");

	AssetID index = _currentAvailableIndex;
	_geometryDescription.insert({ _currentAvailableIndex++, result.desc });

	return index;
}

fs::path AssetManager::ConvertToPath(const fs::path& folder)
{
	fs::path result = fs::current_path();
	while (!helpers::IsProjectRoot(result))
	{
		if (result.has_parent_path())
			result = result.parent_path();
		else
		{
			std::cout << "Unable to convert path! Parent directory not found\n";
			return result;
		}
	}

	result = result / "resources" / "models" / folder;

	return result;
}

fs::path AssetManager::FindGLTFByPath(const fs::path& path)
{
	std::string ext(".gltf");
	for (auto& p : fs::recursive_directory_iterator(path))
	{
		if (p.path().extension() == ext)
			return p.path();
	}

	return {};
}

void AssetManager::UpdateDataPointers()
{
	for (auto& assetDesc : _geometryDescription)
	{
		assetDesc.second.geometryPtr = _storage.GetRawGeometry(assetDesc.first);
		assetDesc.second.indicesPtr = _storage.GetRawIndices(assetDesc.first);
	}
}

const GeometryDescription* AssetManager::GetGeometryDesc(AssetID id) const
{
	auto it = _geometryDescription.find(id);
	if (it == _geometryDescription.end())
	{
		std::cout << "Unable to get geometry desc by id, the data is not stored previously, important error - check call stack! " << id << '\n';
		return nullptr;
	}
	
	return &it->second;
}