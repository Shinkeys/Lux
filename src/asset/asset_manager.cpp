#include "../../headers/asset/asset_manager.h"

// Purpose: used to create vertex buffer for example.
Geometry* AssetManager::GetAllSceneGeometry()
{
	return _storage.GetRawData();
}

size_t AssetManager::GetAllSceneSize()
{
	return _storage.GetRawDataSize();
}

void AssetManager::StoreData(const std::vector<Geometry>& geometry)
{
	_storage.StoreGeometry(geometry);
}