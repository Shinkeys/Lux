#pragma once

#include "../util/util.h"
#include "asset_storage.h"
#include "model_importer.h"

struct MeshStorageBackData
{
	u32 meshIndex{ 0 };
	u32 materialIndex{ 0 };
};

struct MaterialStorageBackData
{
	u32 materialID{ 0 };
	MaterialDescription materials;
};

class VulkanImage;
class ImageManager;
class Image;
class AssetManager
{
private:
	AssetStorage _storage;
	ModelImporter _importer;

	inline static AssetManager* s_Instance;

	using AssetID = u32;
	using MaterialID = u32;
	AssetID _currentAvailableIndex{ 1 }; // 0 is reserved
	MaterialID _availableMaterialIndex{ 1 };

	std::unordered_map<MaterialID, MaterialDescription> _materialDescription;

	std::vector<std::pair<u32, std::unique_ptr<Image>>> _textures;

	fs::path ConvertToPath(const fs::path& folder);
	fs::path FindGLTFByPath(const fs::path& path);
	void ConvertMaterialsPathToAbsolute(const fs::path& modelFolderName, LoadedGLTF& loadedGLTF);
public:

	static void Initialize();
	static void Cleanup();

	AssetManager() = default;
	AssetManager(const AssetManager&) = delete;
	AssetManager(AssetManager&&) = delete;
	AssetManager& operator= (const AssetManager&) = delete;
	AssetManager& operator= (AssetManager&&) = delete;

	static AssetManager* Get() { return s_Instance; }

	const auto& GetAllTextures() const { return _textures; }

	size_t GetAllSceneSize();

	const std::vector<SubmeshDescription>* GetAssetSubmeshes(AssetID id) const;

	const std::vector<MaterialTexturesDesc>& GetAllSceneMaterialsDesc() const;

	/**
	* @brief Write models FOLDER to load the file from it. There's should be files only for one model
	*/
	std::optional<MeshStorageBackData> TryToLoadAndStoreMesh(const fs::path& folder, ImageManager* imageManager = nullptr);
	MaterialTexturesDesc TryToLoadMaterial(const ImageManager& imageManager, const MeshMaterial& material);
	MaterialID StoreLoadedMaterials(const std::vector<MaterialTexturesDesc>& materialsDesc);
};