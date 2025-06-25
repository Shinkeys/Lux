#include "../../headers/asset/asset_storage.h"


StoreGeometryResult AssetStorage::StoreGeometry(const LoadedGLTF& loadedGLTF, AssetID assetID)
{	
	std::vector<Geometry> tempGeometry;
	std::vector<u32> tempIndices;

	StoreGeometryResult result;
	StorageDataOffsets storageOffsets;
	storageOffsets.startGeometry = _allGeometryStorage.size();
	storageOffsets.startIndex = _allIndicesStorage.size();

	u32 geometryTotalSize = 0;
	u32 indexTotalSize = 0;
	for (const auto& mesh : loadedGLTF.meshes)
	{
		const size_t geometrySize = mesh.geometry.size();
		const size_t indicesSize = mesh.indices.size();

		// If storage is reallocated need to update pointers in asset manager class
		if (_allGeometryStorage.capacity() < _allGeometryStorage.size() + geometrySize)
			result.shouldUpdateGeometryPtrs = true;

		if (_allIndicesStorage.capacity() < _allIndicesStorage.size() + indicesSize)
			result.shouldUpdateIndicesPtrs = true;

		_allGeometryStorage.insert(_allGeometryStorage.end(), std::make_move_iterator(mesh.geometry.begin()), std::make_move_iterator(mesh.geometry.end()));
		_allIndicesStorage.insert(_allIndicesStorage.end(), std::make_move_iterator(mesh.indices.begin()), std::make_move_iterator(mesh.indices.end()));

		
		geometryTotalSize += geometrySize;
		indexTotalSize += indicesSize;
	}

	storageOffsets.endGeometry = _allGeometryStorage.empty() ? 0 : _allGeometryStorage.size() - 1;
	storageOffsets.endIndex = _allIndicesStorage.empty() ? 0 : _allIndicesStorage.size() - 1;

	_dataOffsets.emplace(assetID, storageOffsets);

	result.desc.geometryPtr = GetRawGeometry(assetID);
	result.desc.indicesPtr = GetRawIndices(assetID);
	result.desc.geometryCount = geometryTotalSize;
	result.desc.indexCount = indexTotalSize;

	return result;
}

const Geometry* AssetStorage::GetRawGeometry(AssetID id) const
{
	auto it = _dataOffsets.find(id);
	if (it == _dataOffsets.end())
	{
		std::cout << "Unable to get pointer to the raw geometry!\n";
		return nullptr;
	}
	if (it->second.startGeometry >= _allGeometryStorage.size())
	{
		std::cout << "Unable to get pointer to the raw geometry, start geometry index is greater than all geometry size\n";
		return nullptr;
	}

	return &_allGeometryStorage[it->second.startGeometry];
}

const u32* AssetStorage::GetRawIndices(AssetID id)       const
{
	auto it = _dataOffsets.find(id);
	if (it == _dataOffsets.end())
	{
		std::cout << "Unable to get pointer to the raw indices!\n";
		return nullptr;
	}
	if (it->second.startGeometry >= _allGeometryStorage.size())
	{
		std::cout << "Unable to get pointer to the raw indices, start index is greater than all indices size\n";
		return nullptr;
	}

	return &_allIndicesStorage[it->second.startGeometry];
}