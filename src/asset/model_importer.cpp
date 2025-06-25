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

	if (!LoadMeshes(asset.get(), asset->meshes, gltfData))
		return gltfData;

	
	gltfData.isLoaded = true;
	return gltfData;
}


bool ModelImporter::LoadMeshes(const fastgltf::Asset& asset, std::vector<fastgltf::Mesh>& meshes, LoadedGLTF& gltfData)
{
	gltfData.meshes.resize(meshes.size());

	for (u32 i = 0; i < meshes.size(); ++i)
	{
		auto& meshGeometry = gltfData.meshes[i].geometry;
		auto& meshIndices  = gltfData.meshes[i].indices;


		for (auto it = meshes[i].primitives.begin(); it != meshes[i].primitives.end(); ++it)
		{
			auto* positionIt = it->findAttribute("POSITION");
			assert(positionIt != it->attributes.end());
			assert(it->indicesAccessor.has_value()); // Mesh MUST have indices

			auto& positionAccessor = asset.accessors[positionIt->accessorIndex];
			if (!positionAccessor.bufferViewIndex.has_value())
				continue;

			// IMPORTANT
			meshGeometry.resize(positionAccessor.count);

			fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, positionAccessor, [&](fastgltf::math::fvec3 pos, size_t index)
				{
					meshGeometry[index].position = glm::vec3(pos.x(), pos.y(), pos.z());
					meshGeometry[index].normal = glm::vec3();    
					meshGeometry[index].tangent = glm::vec3();   // left it empty in case it doesn't present
					meshGeometry[index].UV = glm::vec2();	 
				});


			// Iterate for normals
			auto* normalIt = it->findAttribute("NORMAL");
			assert(normalIt != it->attributes.end()); // Mesh must have normals
			
			auto& normalAccesor = asset.accessors[normalIt->accessorIndex];
			if (!normalAccesor.bufferViewIndex.has_value())
				continue;

			fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, normalAccesor, [&](fastgltf::math::fvec3 normal, size_t index)
				{
					meshGeometry[index].normal = glm::vec3(normal.x(), normal.y(), normal.z());
				});


			//  to do: UVs


			auto& indicesAccessor = asset.accessors[it->indicesAccessor.value()];
			if (!indicesAccessor.bufferViewIndex.has_value())
				return false;
			// dont care about size, would assume every index is u32
			std::vector<u32> tempIndices(indicesAccessor.count);

			fastgltf::copyFromAccessor<u32>(asset, indicesAccessor, tempIndices.data());
			meshIndices.insert(meshIndices.end(), std::make_move_iterator(tempIndices.begin()), std::make_move_iterator(tempIndices.end()));
		}

	}


	return true;
}