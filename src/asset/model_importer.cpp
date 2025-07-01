#include "../../headers/asset/model_importer.h"

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>

LoadedGLTF ModelImporter::LoadGltf(const fs::path& path)
{
	LoadedGLTF gltfData; // by default is unloaded so can return

	if (!fs::exists(path))
	{
		std::cout << "Failed to find path in ModelImporter: " << path << '\n';
		return gltfData;
	}

	static constexpr auto supportedExtensions =
		fastgltf::Extensions::KHR_mesh_quantization; // to reduce memory usage

	fastgltf::Parser parser(supportedExtensions);
	
	
	using fopt = fastgltf::Options;

	constexpr auto gltfOptions = fopt::DontRequireValidAssetMember | fopt::LoadGLBBuffers | fopt::LoadExternalBuffers | fopt::GenerateMeshIndices;


	auto gltfFile = fastgltf::MappedGltfFile::FromPath(path);
	if (!bool(gltfFile))
	{
		std::cout << "Failed to open GLTF file: " << fastgltf::getErrorMessage(gltfFile.error()) << '\n';
		return gltfData;
	}

	auto asset = parser.loadGltf(gltfFile.get(), path.parent_path(), gltfOptions);

	if (asset.error() != fastgltf::Error::None)
	{
		std::cout << "Failed to open GLTF file: " << fastgltf::getErrorMessage(asset.error()) << '\n';
		return gltfData;
	}

	if (!LoadMeshes(asset.get(), gltfData))
		return gltfData;
	
	gltfData.isLoaded = true;
	return gltfData;
}

void ModelImporter::StoreTextureData(const fastgltf::Image& image, TexturesData& InTexture)
{
	std::visit(fastgltf::visitor{
	[](auto& arg) {},
		[&](fastgltf::sources::URI filePath)
			{
				assert(filePath.uri.isLocalPath());
				// index is 0. change later
				InTexture.path = fs::path(filePath.uri.path().begin(), filePath.uri.path().end());
				InTexture.dataType = TexturesData::Type::URI;
			},
		[&](fastgltf::sources::Array array)
			{
				std::cout << "IT WORKS\n";
				/*gltfData.textures.push_back(LoadedTexture{LoadedTexture::Type::EMBEDDED, 0, "", array.bytes});*/
			},
		[&](fastgltf::sources::BufferView view)
			{
				std::cout << "IT WORKS\n";
			}	
	}, image.data);
}

bool ModelImporter::LoadMeshes(const fastgltf::Asset& asset, LoadedGLTF& gltfData)
{
	gltfData.meshes.resize(asset.meshes.size());

	for (u32 i = 0; i < asset.meshes.size(); ++i)
	{
		auto& meshVertex = gltfData.meshes[i].vertex;
		auto& meshIndices  = gltfData.meshes[i].indices;

		for (auto it = asset.meshes[i].primitives.begin(); it != asset.meshes[i].primitives.end(); ++it)
		{
			auto* positionIt = it->findAttribute("POSITION");
			assert(positionIt != it->attributes.end());
			assert(it->indicesAccessor.has_value()); // Mesh MUST have indices

			auto& positionAccessor = asset.accessors[positionIt->accessorIndex];
			if (!positionAccessor.bufferViewIndex.has_value())
				continue;

			// IMPORTANT
			const size_t vertexOffset = meshVertex.size();
			meshVertex.resize(vertexOffset + positionAccessor.count);

			const u32 materialIndex = it->materialIndex.has_value() ? it->materialIndex.value() : 0;
			fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, positionAccessor, [&](fastgltf::math::fvec3 pos, size_t index)
				{
					meshVertex[index + vertexOffset].position = glm::vec3(pos.x(), pos.y(), pos.z());
					meshVertex[index + vertexOffset].normal = glm::vec3();    
					meshVertex[index + vertexOffset].tangent = glm::vec3();   // left it empty in case it doesn't present
					meshVertex[index + vertexOffset].UV = glm::vec2();

					// Offset those indices later 
					meshVertex[index + vertexOffset].materialIndex = materialIndex;
				});

			size_t baseColorTexCoordIdx = 0;

			if (it->materialIndex.has_value())
			{
				auto& material = asset.materials[it->materialIndex.value()];

				// Albedo
				auto& baseColorTex = material.pbrData.baseColorTexture;
				if (baseColorTex.has_value())
				{
					auto& texture = asset.textures[baseColorTex->textureIndex];
					if (!texture.imageIndex.has_value())
						return false;

					if (baseColorTex->transform && baseColorTex->transform->texCoordIndex.has_value()) {
						baseColorTexCoordIdx = baseColorTex->transform->texCoordIndex.value();
					}
					else {
						baseColorTexCoordIdx = material.pbrData.baseColorTexture->texCoordIndex;
					}
				}

				bool isMaterialAlreadyStored = false;
				for (const auto& material : gltfData.materials)
				{
					if (material.materialIndex == materialIndex)
						isMaterialAlreadyStored = true;
				}

				if (!isMaterialAlreadyStored)
				{
					// Normal
					auto& normalTex = material.normalTexture;
					// MetallicRoughness
					auto& metallicRoughnessTex = material.pbrData.metallicRoughnessTexture;

					MeshMaterial meshMaterial;
					// To verify it. Basically retrieve a path or data for texture and load it later in the asset manager
					if (baseColorTex.has_value())
					{
						auto& baseColorTexture = asset.textures[baseColorTex->textureIndex];

						if (baseColorTexture.imageIndex.has_value())
						{
							const auto& image = asset.images[baseColorTexture.imageIndex.value()];
							TexturesData baseTexReference;
							baseTexReference.textureType = TextureType::TEXTURE_ALBEDO;

							StoreTextureData(image, baseTexReference);

							meshMaterial.materialTextures.emplace_back(std::move(baseTexReference));
						}
					}

					if (normalTex.has_value())
					{
						auto& normalTexture = asset.textures[normalTex->textureIndex];
						if (normalTexture.imageIndex.has_value())
						{
							const auto& image = asset.images[normalTexture.imageIndex.value()];
							TexturesData normalTexReference;
							normalTexReference.textureType = TextureType::TEXTURE_NORMAL;

							StoreTextureData(image, normalTexReference);

							meshMaterial.materialTextures.emplace_back(std::move(normalTexReference));

						}
					}

					if (metallicRoughnessTex.has_value())
					{
						auto& metallicRoughnessTexture = asset.textures[metallicRoughnessTex->textureIndex];

						if (metallicRoughnessTexture.imageIndex.has_value())
						{
							const auto& image = asset.images[metallicRoughnessTexture.imageIndex.value()];
							TexturesData metallicRoughnessTexReference;
							metallicRoughnessTexReference.textureType = TextureType::TEXTURE_METALLICROUGHNESS;

							StoreTextureData(image, metallicRoughnessTexReference);

							meshMaterial.materialTextures.emplace_back(std::move(metallicRoughnessTexReference));

						}
					}

					gltfData.materials.emplace_back(std::move(meshMaterial));
				}
			}
			

			auto texCoordAttribute = std::string("TEXCOORD_") + std::to_string(baseColorTexCoordIdx);
			if (const auto* texCoordIt = it->findAttribute(texCoordAttribute); texCoordIt != it->attributes.end())
			{
				auto& texCoordAccessor = asset.accessors[texCoordIt->accessorIndex];

				if (!texCoordAccessor.bufferViewIndex.has_value())
					continue;

				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset, texCoordAccessor, [&](fastgltf::math::fvec2 uv, std::size_t index) {
						meshVertex[index + vertexOffset].UV = glm::vec2(uv.x(), uv.y());
					});
			}


			// Iterate for normals
			auto* normalIt = it->findAttribute("NORMAL");
			assert(normalIt != it->attributes.end()); // Mesh must have normals
			
			auto& normalAccesor = asset.accessors[normalIt->accessorIndex];
			if (!normalAccesor.bufferViewIndex.has_value())
				continue;

			fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, normalAccesor, [&](fastgltf::math::fvec3 normal, size_t index)
				{
					meshVertex[index + vertexOffset].normal = glm::vec3(normal.x(), normal.y(), normal.z());
				});

	
			auto& indicesAccessor = asset.accessors[it->indicesAccessor.value()];
			if (!indicesAccessor.bufferViewIndex.has_value())
				return false;
			// dont care about size, would assume every index is u32
			std::vector<u32> tempIndices(indicesAccessor.count);

			fastgltf::copyFromAccessor<u32>(asset, indicesAccessor, tempIndices.data());
			for (u32& index : tempIndices)
			{
				index += static_cast<u32>(vertexOffset);
			}
			meshIndices.insert(meshIndices.end(), std::make_move_iterator(tempIndices.begin()), std::make_move_iterator(tempIndices.end()));
		}
	}
	return true;
}