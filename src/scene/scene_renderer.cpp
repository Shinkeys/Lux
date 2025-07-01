#include "../../headers/scene/scene_renderer.h"
#include "../../headers/scene/entity.h"

#include <glm/gtc/matrix_transform.hpp>

SceneRenderer::SceneRenderer(VulkanBase& vulkanBackend, AssetManager& manager) : _vulkanBackend{vulkanBackend}, _assetManager{manager}
{
	// Descriptor
	// Create descriptor for every frame
	for (u32 i = 0; i < VulkanFrame::FramesInFlight; ++i)
	{
		constexpr i32 descriptorUsageCount = 1;
		DescriptorInfo descInfo{};
		descInfo.bindings.resize(descriptorUsageCount);
		descInfo.bindings[0].binding = 0;
		descInfo.bindings[0].descriptorCount = 1024; // BINDLESS. SPECIFY LATER
		descInfo.bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descInfo.bindings[0].stageFlags = VK_SHADER_STAGE_ALL; // bindless buffer of textures
		_baseShadingDescriptorSets.emplace_back(vulkanBackend.GetDescriptorObj().CreateDescSet(descInfo));
	}

	
	 
	constexpr i32 pushConstBufferCount = 3; // uniform + vertexBuff + ssbo

	GraphicsPipeline baseShadingPipeline;
	baseShadingPipeline.shaderName = "shading";
	baseShadingPipeline.cullMode = VK_CULL_MODE_BACK_BIT;
	baseShadingPipeline.pushConstantSizeBytes = sizeof(VkDeviceAddress) * pushConstBufferCount;
	baseShadingPipeline.descriptorLayouts = { _vulkanBackend.GetDescriptorObj().GetBindlessDescriptorSetLayout() }; // Now only one descriptor layout, to DO

	// other data is aight

	_baseShadingPair = _vulkanBackend.GetPipelineObj().CreatePipeline(baseShadingPipeline);



	// Create sampler
	VkFilter minFiltering{ VK_FILTER_LINEAR };
	VkFilter magFiltering{ VK_FILTER_LINEAR };
	VkSamplerMipmapMode mipmapFiltering{ VK_SAMPLER_MIPMAP_MODE_LINEAR };
	VkSamplerAddressMode addressMode{ VK_SAMPLER_ADDRESS_MODE_REPEAT };
	float minLod{ 0.0f };
	float maxLod{ 100.0f };
	float lodBias{ 0.0f };

	CreateSamplerSpec samplerSpec;
	samplerSpec.minFiltering = VK_FILTER_LINEAR;
	samplerSpec.magFiltering = VK_FILTER_LINEAR;
	samplerSpec.mipmapFiltering = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerSpec.addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerSpec.minLod = 0.0f;
	samplerSpec.maxLod = 1000.0f;
	samplerSpec.lodBias = 0.0f;


	_samplerLinear = _vulkanBackend.GetImageObj().CreateSampler(samplerSpec);
}


void SceneRenderer::UpdateBuffers(const Entity& entity)
{
	VulkanBuffer& bufferManager = _vulkanBackend.GetBufferObj();
	VulkanImage& imageManager = _vulkanBackend.GetImageObj();

	MeshComponent* meshComp = entity.GetComponent<MeshComponent>();
	// Check if buffer exist
	if (meshComp != nullptr && bufferManager.GetMeshBuffers(entity.GetID()) == nullptr)
	{
		auto loadingResult = _assetManager.TryToLoadAndStoreMesh(meshComp->folderName);
		if (!loadingResult.has_value())
			return;

		u32 meshIndex = loadingResult->assetID;
		meshComp->meshIndex = meshIndex;
		if (meshIndex != 0)
		{
			const VertexDescription* desc = _assetManager.GetVertexDesc(meshIndex);
			if (desc != nullptr)
			{
				MeshVertexBufferCreateDesc vertexDesc;
				vertexDesc.vertexPtr = desc->vertexPtr;
				vertexDesc.elementsCount = desc->vertexCount;
				vertexDesc.bufferUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
				vertexDesc.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				vertexDesc.bufferMemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

				MeshIndexBufferCreateDesc indexDesc;
				indexDesc.indicesPtr = desc->indicesPtr;
				indexDesc.elementsCount = desc->indexCount;
				indexDesc.bufferUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
				indexDesc.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				indexDesc.bufferMemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

				bufferManager.CreateMeshBuffers(vertexDesc, indexDesc, entity.GetID());
			}
		}
		else
		{
			std::cout << "Unable to create mesh buffers, asset manager returned index 0\n";
		}

		// MATERIAL
		std::vector<MaterialTexturesDesc> allMeshMaterials;
		for (const auto& material : loadingResult->unloadedMaterials)
		{
			allMeshMaterials.push_back(_assetManager.TryToLoadMaterial(imageManager, material));
		}

		const auto materialsStoreResult = 	_assetManager.StoreLoadedMaterials(allMeshMaterials);
		meshComp->materialIndex = materialsStoreResult.materialID;
	}

	const bool isUniformCreated = bufferManager.GetUniformBuffer(entity.GetID()) != nullptr;
	if (isUniformCreated)
	{
		const CameraComponent* comp = entity.GetComponent<CameraComponent>();
		const TranslationComponent* transComp = entity.GetComponent<TranslationComponent>();
		if (comp && comp->isActive)
		{
			glm::mat4 model = transComp ? GenerateModelMatrix(*transComp) : glm::mat4(1.0f);
			UniformData uniform{};
			uniform.view = comp->camera->GetViewMatrix();
			uniform.proj = comp->camera->GetProjectionMatrix();
			uniform.model = model;
			bufferManager.UpdateUniformBuffer(uniform, sizeof(UniformData), entity.GetID());
		}
	}
	else // create
	{
		UniformData uniform{};
		const CameraComponent* comp = entity.GetComponent<CameraComponent>();
		const TranslationComponent* transComp = entity.GetComponent<TranslationComponent>();
		if (comp && comp->isActive)
		{
			glm::mat4 model = transComp ? GenerateModelMatrix(*transComp) : glm::mat4(1.0f);
			uniform.view = comp->camera->GetViewMatrix();
			uniform.proj = comp->camera->GetProjectionMatrix();
			uniform.model = model;
		}

		bufferManager.CreateUniformBuffer(uniform, sizeof(UniformData), entity.GetID());
	}
}

// Return value: NRVO would return it so no copy needed.
glm::mat4 SceneRenderer::GenerateModelMatrix(const TranslationComponent& translationComp)
{
	glm::mat4 model(1.0f);
	model = glm::scale(model, translationComp.scale);
	model = translationComp.rotation * model;
	model = glm::translate(model, translationComp.translation);

	return model;
}

void SceneRenderer::Update() const
{
	VulkanBuffer& bufferManager = _vulkanBackend.GetBufferObj();
	VulkanFrame& frameManager = _vulkanBackend.GetFrameObj();
	VulkanImage& imageManager = _vulkanBackend.GetImageObj();
	VulkanDescriptor& descriptorManager = _vulkanBackend.GetDescriptorObj();


	for (u32 descInd = 0; descInd < VulkanFrame::FramesInFlight; ++descInd)
	{
		const std::vector<ImageHandle>& images = imageManager.GetAllImages(); // TEMPORARY SOLUTION. TO REWORK
		for (u32 i = 0; i < images.size(); ++i)
		{
			DescriptorUpdate updateData;
			updateData.set = _baseShadingDescriptorSets[descInd]; // to replace
			updateData.imgView = images[i].imageView;
			updateData.dstBinding = 0;
			updateData.dstArrayElem = images[i].index; // starting from 1, zero is null

			updateData.sampler = _samplerLinear;

			descriptorManager.UpdateBindlessDescriptorSet(updateData);
		}
	}
}

void SceneRenderer::Draw()
{
	VulkanFrame& frameManager = _vulkanBackend.GetFrameObj();
	VulkanBuffer& bufferManager = _vulkanBackend.GetBufferObj();

	while (!_drawCommands.empty())
	{
		const auto& command = _drawCommands.front();
		frameManager.SubmitRenderTask(command); // Example. TO DO
		_drawCommands.pop();
	}
}

void SceneRenderer::SubmitEntityToDraw(const Entity& entity)
{
	VulkanBuffer& bufferManager = _vulkanBackend.GetBufferObj();
	VulkanFrame& frameManager = _vulkanBackend.GetFrameObj();

	UpdateBuffers(entity);


	const MeshBuffers* meshBuffers = bufferManager.GetMeshBuffers(entity.GetID());
	assert(meshBuffers && "Mesh buffer for entity is empty");
	const StorageBuffer* uniformBuffer = bufferManager.GetUniformBuffer(entity.GetID());
	assert(meshBuffers && "Uniform buffer for entity is empty");


	if (_baseMaterialsSSBO.address == 0)
	{
		const auto& materials = _assetManager.GetAllSceneMaterialsDesc();

		_baseMaterialsSSBO = bufferManager.CreateSSBOBuffer(materials);

		std::cout << "Address on creation: " << static_cast<u64>(_baseMaterialsSSBO.address) << '\n';
	}

	const MeshComponent* meshComp = entity.GetComponent<MeshComponent>();
	if (meshComp != nullptr)
	{
		const u32 meshAssetIndex = meshComp->meshIndex;
		if (meshAssetIndex == 0)
			return;

		const VertexDescription* vertexDesc =  _assetManager.GetVertexDesc(meshAssetIndex);
		if (vertexDesc == nullptr)
			return;

		VulkanDrawCommand command;
		command.pipeline = _baseShadingPair.pipeline;
		command.pipelineLayout = _baseShadingPair.pipelineLayout;
		command.descriptorSet = _baseShadingDescriptorSets[frameManager.GetCurrentFrameIndex()].descriptorSet;
		command.buffersAddresses = { meshBuffers->GetDeviceAddress(), uniformBuffer->GetDeviceAddress(), _baseMaterialsSSBO.address };
		command.indexCount = vertexDesc->indexCount;
		command.indexBuffer = bufferManager.GetMeshBuffers(entity.GetID())->GetVkIndexBuffer();
		command.type = RenderJobType::GEOMETRY_PASS;
		_drawCommands.emplace(std::move(command));

	}

}