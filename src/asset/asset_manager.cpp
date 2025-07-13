#include "../../headers/asset/asset_manager.h"
#include "../../headers/base/core/image.h"
#include "../../headers/base/gfx/vk_image.h"
#include "../../headers/util/helpers.h"




// Purpose: used to create vertex buffer for example.
void AssetManager::Initialize()
{
	s_Instance = new AssetManager;
}


void AssetManager::Cleanup()
{
	delete s_Instance;
}



size_t AssetManager::GetAllSceneSize()
{
	return _storage.GetRawDataSize();
}

// WORKS ONLY WITH GLTF
// TO REWORK THIS CLASS A LITTLE BIT
std::optional<MeshStorageBackData> AssetManager::TryToLoadAndStoreMesh(const fs::path& folder)
{
	fs::path pathToLoad = ConvertToPath(folder);
	fs::path finalPath = FindGLTFByPath(pathToLoad);

	LoadedGLTF loadedGLTF = _importer.LoadGltf(finalPath);
	if (!loadedGLTF.isLoaded)
	{
		std::cout << "Unable to load model by provided folder: " << folder << '\n';
		return std::nullopt;
	}

	AssetID index = _currentAvailableIndex;
	StoreVertexResult result = _storage.StoreVertex(loadedGLTF, index);
	if (result.shouldUpdateVertexPtrs || result.shouldUpdateIndicesPtrs) // update em both at once. doesnt affect performance anyway
		UpdateDataPointers();
	
	assert(result.desc.vertexCount > 0 && result.desc.indexCount > 0 && "Vertex or index count is 0 in TryToLoadAndStoreMesh(). Check the logic");
	assert(result.desc.vertexPtr != nullptr && result.desc.indicesPtr != nullptr && "Vertex ptr or indices ptr is nullptr in TryToLoadAndStoreMesh(). Check the logic");

	ConvertMaterialsPathToAbsolute(folder, loadedGLTF);

	_vertexDescription.insert({ index, result.desc });

	++_currentAvailableIndex;

	MeshStorageBackData backData;
	backData.assetID = index;
	backData.unloadedMaterials = std::move(loadedGLTF.materials);

	return backData;
}

MaterialStorageBackData AssetManager::StoreLoadedMaterials(const std::vector<MaterialTexturesDesc>& materialsDesc)
{
	const u32 availableIndex = _availableMaterialIndex;
	++_availableMaterialIndex;
	StoreMaterialResult storeResult = _storage.StoreMeshMaterials(materialsDesc, availableIndex);

	if (storeResult.shouldUpdatePtrs)
		UpdateMaterialPointers();

	MaterialStorageBackData backData;
	backData.materialID = availableIndex;
	backData.materials = storeResult.desc;

	return backData;
}

MaterialTexturesDesc AssetManager::TryToLoadMaterial(const ImageManager& imageManager, const MeshMaterial& material)
{
	MaterialTexturesDesc description;
	for (auto& texture : material.materialTextures)
	{
		ImageSpecification spec;
		spec.type = ImageType::IMAGE_TYPE_TEXTURE;
		spec.aspect = ImageAspect::IMAGE_ASPECT_COLOR;
		spec.format = ImageFormat::IMAGE_FORMAT_R8G8B8A8_SRGB;
		spec.path = texture.path;

		const u32 availableIndex = static_cast<u32>(_textures.size() + 1); // starts from 1

		switch (texture.dataType)
		{
		case TexturesData::Type::URI:
		{
			if (texture.textureType == TextureType::TEXTURE_ALBEDO)
			{
				auto res = imageManager.CreateImage(spec);
				if (res)
				{
					description.albedoID = availableIndex;
					description.baseColorFactor = material.baseColorFactor;
					_textures.emplace_back(availableIndex, std::move(res));
				}
				else
					description.albedoID = 0;
			}
			else if (texture.textureType == TextureType::TEXTURE_NORMAL)
			{
				auto res = imageManager.CreateImage(spec);
				if (res)
				{
					description.normalID = availableIndex;
					_textures.emplace_back(availableIndex, std::move(res));
				}
				else
					description.normalID = 0;
			}
			else if (texture.textureType == TextureType::TEXTURE_METALLICROUGHNESS)
			{
				auto res = imageManager.CreateImage(spec);
				if (res)
				{
					description.metalRoughnessID = availableIndex;
					description.metallicFactor = material.metallicFactor;
					description.roughnessFactor = material.roughnessFactor;
					_textures.emplace_back(availableIndex, std::move(res));
				}
				else
					description.metalRoughnessID = 0;
			}

			break;
		}

		case TexturesData::Type::EMBEDDED:
		{
			// to do;
			break;
		}

		default: std::cout << "Unknown data type of texture\n";
			break;
		}
	}

	return description;
}


// Just a helper function to make it more approachable in the code
// When storing mesh textures need to convert all texture paths
// to the absolute to load later.
void AssetManager::ConvertMaterialsPathToAbsolute(const fs::path& modelFolderName, LoadedGLTF& loadedGLTF)
{
	for (auto& material : loadedGLTF.materials)
	{
		for (auto& texture : material.materialTextures)
		{
			texture.path = ConvertToPath(modelFolderName / texture.path);
		}
	}
}

const std::vector<MaterialTexturesDesc>& AssetManager::GetAllSceneMaterialsDesc() const
{
	return _storage.GetAllSceneMaterials();
}

fs::path AssetManager::ConvertToPath(const fs::path& folder)
{
	fs::path result = fs::current_path();
	while (!helpers::IsProjectRoot(result))
	{
		if (result.has_parent_path())
			result = result.parent_path();
		else
		{
			std::cout << "Unable to convert path! Parent directory not found\n";
			return result;
		}
	}

	result = result / "resources" / "models" / folder;

	return result;
}

fs::path AssetManager::FindGLTFByPath(const fs::path& path)
{
	std::string ext(".gltf");
	for (auto& p : fs::recursive_directory_iterator(path))
	{
		if (p.path().extension() == ext)
			return p.path();
	}

	return {};
}

void AssetManager::UpdateDataPointers()
{
	for (auto& assetDesc : _vertexDescription)
	{
		assetDesc.second.vertexPtr = _storage.GetRawVertex(assetDesc.first);
		assetDesc.second.indicesPtr = _storage.GetRawIndices(assetDesc.first);
	}
}

void AssetManager::UpdateMaterialPointers()
{
	for (auto& materialDesc : _materialDescription)
	{
		materialDesc.second.materialTexturesPtr = _storage.GetRawMaterials(materialDesc.first);
	}
}

const VertexDescription* AssetManager::GetVertexDesc(AssetID id) const
{
	auto it = _vertexDescription.find(id);
	if (it == _vertexDescription.end())
	{
		std::cout << "Unable to get geometry desc by id, the data is not stored previously, important error - check call stack! " << id << '\n';
		return nullptr;
	}
	
	return &it->second;
}
