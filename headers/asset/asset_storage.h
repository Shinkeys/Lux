#pragma once
#include "../util/util.h"
#include "../scene/component.h"
#include "asset_types.h"

#include <glm/glm.hpp>

struct StorageDataOffsets
{
	i32 startGeometry{ 0 };
	i32 endGeometry{ 0 };
	i32 startIndex{ 0 };
	i32 endIndex{ 0 };
};

struct StoreGeometryResult
{
	GeometryDescription desc;
	bool shouldUpdateGeometryPtrs{ false };
	bool shouldUpdateIndicesPtrs{ false };
};

class AssetStorage
{
private:
	std::vector<Geometry> _allGeometryStorage;
	std::vector<u32> _allIndicesStorage;

	using AssetID = u32;
	std::unordered_map<AssetID, StorageDataOffsets> _dataOffsets;
public:	
	const Geometry* GetRawGeometry(AssetID id) const;
	const u32* GetRawIndices(AssetID id)       const;
	size_t GetRawDataSize() const { return _allGeometryStorage.size(); }
	StoreGeometryResult StoreGeometry(const LoadedGLTF& loadedGltf, AssetID assetID);
};