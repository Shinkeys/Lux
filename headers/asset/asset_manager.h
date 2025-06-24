#pragma once

#include "../util/util.h"
#include "asset_storage.h"



class AssetManager
{
private:
	AssetStorage _storage;
public:
	Geometry* GetAllSceneGeometry();
	size_t GetAllSceneSize();
	void StoreData(const std::vector<Geometry>& geometry); // to remove
};