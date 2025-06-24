#include "../../headers/asset/asset_storage.h"


void AssetStorage::StoreGeometry(const std::vector<Geometry>& geometry)
{
	_allGeometryStorage.insert(_allGeometryStorage.end(), geometry.begin(), geometry.end());
}

