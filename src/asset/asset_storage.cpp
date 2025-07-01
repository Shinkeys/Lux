#include "../../headers/asset/asset_storage.h"


StoreVertexResult AssetStorage::StoreVertex(const LoadedGLTF& loadedGLTF, AssetID assetID)
{	
	StoreVertexResult result;
	GeometryDataProps geometryProps;

	u32 vertexTotalSize = 0;
	u32 indexTotalSize = 0;

	std::vector<Vertex> tempVertexStorage;
	std::vector<u32> tempIndexStorage;
	for (const auto& mesh : loadedGLTF.meshes)
	{
		const size_t vertexSize = mesh.vertex.size();
		const size_t indicesSize = mesh.indices.size();

		tempVertexStorage.insert(tempVertexStorage.end(), std::make_move_iterator(mesh.vertex.begin()), std::make_move_iterator(mesh.vertex.end()));
		tempIndexStorage.insert(tempIndexStorage.end(), std::make_move_iterator(mesh.indices.begin()), std::make_move_iterator(mesh.indices.end()));

		
		vertexTotalSize += vertexSize;
		indexTotalSize += indicesSize;
	}
	// If storage is reallocated need to update pointers in asset manager class
	if (_allVertexStorage.capacity() < _allVertexStorage.size() + vertexTotalSize)
		result.shouldUpdateVertexPtrs = true;

	if (_allIndicesStorage.capacity() < _allIndicesStorage.size() + indexTotalSize)
		result.shouldUpdateIndicesPtrs = true;

	assert(!tempVertexStorage.empty() && !tempIndexStorage.empty() && "Trying to store some model with empty vertices/indices");

	geometryProps.vertexCount = vertexTotalSize;
	geometryProps.indexCount = indexTotalSize;

	const u32 insertVertexIndex = _allVertexStorage.size();
	const u32 insertIndex = _allIndicesStorage.size();

	// Insert all the data into storages
	_allVertexStorage.insert(_allVertexStorage.end(), std::make_move_iterator(tempVertexStorage.begin()), std::make_move_iterator(tempVertexStorage.end()));
	_allIndicesStorage.insert(_allIndicesStorage.end(), std::make_move_iterator(tempIndexStorage.begin()), std::make_move_iterator(tempIndexStorage.end()));


	geometryProps.startVertex = &_allVertexStorage[insertVertexIndex];
	geometryProps.startIndex = &_allIndicesStorage[insertIndex];


	if (result.shouldUpdateVertexPtrs || result.shouldUpdateIndicesPtrs)
		UpdateGeometryPropsStorage();


	_geometryProps.emplace(assetID, geometryProps);

	result.desc.vertexPtr = geometryProps.startVertex;
	result.desc.indicesPtr = geometryProps.startIndex;
	result.desc.vertexCount = vertexTotalSize;
	result.desc.indexCount = indexTotalSize;

	return result;
}

void AssetStorage::UpdateGeometryPropsStorage()
{
	size_t vertexIndex = 0;
	size_t indicesIndex = 0;
	for (auto& data : _geometryProps)
	{
		data.second.startVertex = &_allVertexStorage[vertexIndex];
		data.second.startIndex = &_allIndicesStorage[indicesIndex];

		vertexIndex += data.second.vertexCount;
		indicesIndex += data.second.indexCount;

		assert(vertexIndex > 0 && indicesIndex > 0 && "Trying to update ptr to the data in asset storage, but some model contains <= 0 elements");
	}
}

void AssetStorage::UpdateMaterialPropsStorage()
{
	size_t materialIndex = 0;
	for (auto& data : _createdMaterialsProps)
	{
		data.second.desc.materialTexturesPtr = &_allCreatedMaterialsStorage[materialIndex];

		materialIndex += data.second.desc.materialsCount;

		assert(materialIndex > 0 && "Trying to update stored materials props, but some descriptor contains <= 0 elements");
	}
}


StoreMaterialResult AssetStorage::StoreMeshMaterials(const std::vector<MaterialTexturesDesc>& materialsDesc, MaterialsAssetID assetID)
{
	StoreMaterialResult result;

	size_t indexBeforeInsertion = _allCreatedMaterialsStorage.size();
	for (const auto& material : materialsDesc)
	{
		if (_allCreatedMaterialsStorage.capacity() < _allCreatedMaterialsStorage.size() + 1)
		{
			const size_t currentCapacity = _allCreatedMaterialsStorage.capacity();
			const size_t exponentedCapacity = currentCapacity + currentCapacity / 2;
			_allCreatedMaterialsStorage.reserve(exponentedCapacity); // to avoid a lot of reallocations
			result.shouldUpdatePtrs = true;
		}
		_allCreatedMaterialsStorage.push_back(material);
	}
	
	assert(_allCreatedMaterialsStorage.size() > 0 && "Trying to store materials data for some mesh, but it is empty");

	result.desc.materialTexturesPtr = &_allCreatedMaterialsStorage[indexBeforeInsertion];
	result.desc.materialsCount = materialsDesc.size();

	_createdMaterialsProps.emplace(assetID, result);

	if (result.shouldUpdatePtrs)
		UpdateMaterialPropsStorage();


	return result;
}

const MaterialTexturesDesc* AssetStorage::GetRawMaterials(MaterialsAssetID id) const
{
	auto it = _createdMaterialsProps.find(id);
	if (it == _createdMaterialsProps.end())
	{
		std::cout << "Unable to get pointer to the raw materials!\n";
		return nullptr;
	}

	return it->second.desc.materialTexturesPtr;
}

const Vertex* AssetStorage::GetRawVertex(AssetID id) const
{
	auto it = _geometryProps.find(id);
	if (it == _geometryProps.end())
	{
		std::cout << "Unable to get pointer to the raw geometry!\n";
		return nullptr;
	}

	return it->second.startVertex;
}

const u32* AssetStorage::GetRawIndices(AssetID id) const
{
	auto it = _geometryProps.find(id);
	if (it == _geometryProps.end())
	{
		std::cout << "Unable to get pointer to the raw indices!\n";
		return nullptr;
	}

	return it->second.startIndex;
}