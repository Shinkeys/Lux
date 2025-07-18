#pragma once
#include "../util/util.h"
#include "../scene/component.h"
#include "asset_types.h"

#include <glm/glm.hpp>


struct StoreVertexResult
{
	std::vector<SubmeshDescription> desc;
	/*bool shouldUpdateVertexPtrs{ false };
	bool shouldUpdateIndicesPtrs{ false };*/
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
	std::map<AssetID, std::vector<SubmeshDescription>> _submeshesDesc;
	std::map<MaterialsAssetID, StoreMaterialResult> _createdMaterialsProps;
	void UpdateGeometryPropsStorage();
	void UpdateMaterialPropsStorage();
public:	
	const std::vector<MaterialTexturesDesc>& GetAllSceneMaterials() const { return _allCreatedMaterialsStorage; }
	const MaterialTexturesDesc* GetRawMaterials(MaterialsAssetID id) const;
	size_t GetRawDataSize() const { return _allVertexStorage.size(); }
	void StoreVertex(const LoadedGLTF& loadedGltf, AssetID assetID);
	StoreMaterialResult StoreMeshMaterials(const std::vector<MaterialTexturesDesc>& materialsDesc, MaterialsAssetID assetID);

	const std::vector<SubmeshDescription>* GetAssetSubmeshes(AssetID id) const;

	const std::vector<MeshMaterial>& GetAllSceneMaterialsData() const { return _allUnloadedMaterialsStorage; }
};