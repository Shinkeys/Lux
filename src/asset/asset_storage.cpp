#include "../../headers/asset/asset_storage.h"


void AssetStorage::StoreVertex(const LoadedGLTF& loadedGLTF, AssetID assetID)
{
	StoreVertexResult result{};
	result.desc.resize(loadedGLTF.meshes.size());

	for (u32 i = 0; i < loadedGLTF.meshes.size(); ++i)
	{
		const auto& mesh = loadedGLTF.meshes[i];
		const size_t vertexSize = mesh.vertex.size();
		const size_t indicesSize = mesh.indices.size();

		assert(vertexSize > 0 && indicesSize && "Trying to store some model with empty vertices/indices");

		//// If storage is reallocated need to update pointers in asset manager class
		//if (_allVertexStorage.capacity() < _allVertexStorage.size() + vertexSize)
		//	result.shouldUpdateVertexPtrs = true;

		//if (_allIndicesStorage.capacity() < _allIndicesStorage.size() + indicesSize)
		//	result.shouldUpdateIndicesPtrs = true;

		// Insert all the data into storages
		_allVertexStorage.insert(_allVertexStorage.end(), std::make_move_iterator(mesh.vertex.begin()), std::make_move_iterator(mesh.vertex.end()));
		_allIndicesStorage.insert(_allIndicesStorage.end(), std::make_move_iterator(mesh.indices.begin()), std::make_move_iterator(mesh.indices.end()));


		// Store geometry data properties to retrieve them later if would need from this class.
		result.desc[i].vertexDesc.vertexPtr = nullptr;
		result.desc[i].vertexDesc.indicesPtr = nullptr;
		result.desc[i].vertexDesc.vertexCount = vertexSize;
		result.desc[i].vertexDesc.indexCount = indicesSize;
		result.desc[i].alphaMode = mesh.alphaMode;	


		if (auto it = _submeshesDesc.find(assetID); it != _submeshesDesc.end())
			it->second.emplace_back(result.desc[i]);
		else
			_submeshesDesc.insert({ assetID, {result.desc[i]} });
	}

	
	UpdateGeometryPropsStorage();
}

const std::vector<SubmeshDescription>* AssetStorage::GetAssetSubmeshes(AssetID id) const
{
	auto it = _submeshesDesc.find(id);

	if (it == _submeshesDesc.end())
		return nullptr;

	return &it->second;
}

// Purpose: method to update all the pointers to the data inside of storage after reallocation
void AssetStorage::UpdateGeometryPropsStorage()
{
	size_t vertexIndex = 0;
	size_t indicesIndex = 0;
	for (auto& meshes : _submeshesDesc)
	{
		for (auto& mesh : meshes.second)
		{
			mesh.vertexDesc.vertexPtr  = &_allVertexStorage[vertexIndex];
			mesh.vertexDesc.indicesPtr = &_allIndicesStorage[indicesIndex];


			vertexIndex  += mesh.vertexDesc.vertexCount;
			indicesIndex += mesh.vertexDesc.indexCount;

			assert(vertexIndex > 0 && indicesIndex > 0 && "Trying to update ptr to the data in asset storage, but some model contains <= 0 elements");
		}
	}
}

void AssetStorage::UpdateMaterialPropsStorage()
{
	size_t materialIndex = 0;
	for (auto& data : _submeshesDesc)
	{
		for(auto& submesh : data.second)
		{
			submesh.materialDesc.materialTexturesPtr = &_allCreatedMaterialsStorage[materialIndex];
			
			materialIndex += submesh.materialDesc.materialsCount;

			assert(materialIndex > 0 && "Trying to update stored materials props, but some descriptor contains <= 0 elements");
		}
	}
}


void AssetStorage::StoreMeshMaterials(const std::vector<MaterialTexturesDesc>& materialsDesc, MaterialsAssetID assetID)
{
	for (u32 i = 0; i < materialsDesc.size(); ++i)
	{
		StoreMaterialResult result{};

		if (_allCreatedMaterialsStorage.capacity() < _allCreatedMaterialsStorage.size() + 1)
		{
			const size_t currentCapacity = _allCreatedMaterialsStorage.capacity();
			const size_t exponentedCapacity = currentCapacity + currentCapacity / 2;
			_allCreatedMaterialsStorage.reserve(exponentedCapacity); // to avoid a lot of reallocations
		}

		result.desc.materialTexturesPtr = nullptr;
		result.desc.materialsCount = 1; // 1 material per submesh

		_allCreatedMaterialsStorage.push_back(materialsDesc[i]);


		if (auto it = _submeshesDesc.find(assetID); it != _submeshesDesc.end())
		{
			if (i < it->second.size())
			{
				it->second[i].materialDesc = result.desc;
			}
			else
			{
				SubmeshDescription submeshDesc{};
				submeshDesc.materialDesc = result.desc;
				it->second.push_back(submeshDesc);
			}
		}
		else
		{
			SubmeshDescription submeshDesc{};
			submeshDesc.materialDesc = result.desc;
			_submeshesDesc.insert({ assetID, { submeshDesc } });
		}

	}

	assert(_allCreatedMaterialsStorage.size() > 0 && "Trying to store materials data for some mesh, but it is empty");

	UpdateMaterialPropsStorage();

}
