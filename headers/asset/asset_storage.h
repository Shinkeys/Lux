#pragma once
#include "../util/util.h"
#include "../scene/component.h"
#include "asset_types.h"

#include <glm/glm.hpp>

struct GeometryDataProps
{
	Vertex* startVertex{ nullptr };
	u32 vertexCount{ 0 };
	u32* startIndex{ nullptr };
	u32 indexCount{ 0 };
};

struct StoreVertexResult
{
	VertexDescription desc;
	bool shouldUpdateVertexPtrs{ false };
	bool shouldUpdateIndicesPtrs{ false };
};

struct StoreMaterialResult
{
	MaterialDescription desc;
	bool shouldUpdatePtrs{ false };
};

struct UnloadedMaterialsDataProps
{
	MeshMaterial* startPtr{ nullptr };
	u32 elementsCount{ 0 };
	bool shouldUpdatePtrs{ false };
};

class AssetStorage
{
private:
	std::vector<Vertex> _allVertexStorage;
	std::vector<u32> _allIndicesStorage;

	std::vector<MeshMaterial> _allUnloadedMaterialsStorage; // storage to handle materials paths/data to load
	std::vector<MaterialTexturesDesc> _allCreatedMaterialsStorage; // storage to handle textures inside of materials
	
	using AssetID = u32;
	using MaterialsAssetID = u32;
	using ElementsBefore = u32;
	std::map<AssetID, GeometryDataProps> _geometryProps;
	std::map<MaterialsAssetID, StoreMaterialResult> _createdMaterialsProps;
	void UpdateGeometryPropsStorage();
	void UpdateMaterialPropsStorage();
public:	
	const std::vector<MaterialTexturesDesc>& GetAllSceneMaterials() const { return _allCreatedMaterialsStorage; }
	const Vertex* GetRawVertex(AssetID id) const;
	const u32* GetRawIndices(AssetID id)       const;
	const MaterialTexturesDesc* GetRawMaterials(MaterialsAssetID id) const;
	size_t GetRawDataSize() const { return _allVertexStorage.size(); }
	StoreVertexResult StoreVertex(const LoadedGLTF& loadedGltf, AssetID assetID);
	StoreMaterialResult StoreMeshMaterials(const std::vector<MaterialTexturesDesc>& materialsDesc, MaterialsAssetID assetID);

	const std::vector<MeshMaterial>& GetAllSceneMaterialsData() const { return _allUnloadedMaterialsStorage; }
};