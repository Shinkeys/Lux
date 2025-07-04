#include "../../headers/scene/scene_renderer.h"
#include "../../headers/scene/entity.h"

#include <glm/gtc/matrix_transform.hpp>

SceneRenderer::SceneRenderer(VulkanBase& vulkanBackend) : _vulkanBackend{ vulkanBackend }
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



	constexpr i32 pushConstBufferCount = 10; // uniform + vertexBuff + ssbo

	GraphicsPipeline baseShadingPipeline;
	baseShadingPipeline.shaderName = "shading";
	baseShadingPipeline.cullMode = VK_CULL_MODE_BACK_BIT;
	baseShadingPipeline.pushConstantSizeBytes = sizeof(VkDeviceAddress) * pushConstBufferCount;
	baseShadingPipeline.descriptorLayouts = { _vulkanBackend.GetDescriptorObj().GetBindlessDescriptorSetLayout() }; // Now only one descriptor layout, to DO
	baseShadingPipeline.depthCompare = VK_COMPARE_OP_LESS;
	baseShadingPipeline.depthWriteEnable = VK_TRUE;
	baseShadingPipeline.depthTestEnable = VK_TRUE;

	// other data is aight

	_baseShadingPair = _vulkanBackend.GetPipelineObj().CreatePipeline(baseShadingPipeline);
	_depthAttachments.resize(VulkanPresentation::PresentationImagesCount);
	for (u32 i = 0; i < VulkanPresentation::PresentationImagesCount; ++i)
	{
		ImageSpecification spec;
		spec.mipLevels = 1;
		spec.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		spec.aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
		spec.format = VK_FORMAT_D32_SFLOAT;
		spec.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		spec.extent = VkExtent3D{ static_cast<u32>(Window::GetWindowWidth()), static_cast<u32>(Window::GetWindowHeight()), 1 };
		spec.aspect = VK_IMAGE_ASPECT_DEPTH_BIT;

		_depthAttachments[i] = _vulkanBackend.GetImageObj().CreateEmptyImage(spec);
	}

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

	// Materials buffer
	constexpr size_t baseMaterialsBuffersSize = 1024 * 4; // 4096 bytes
	_baseMaterialsSSBO = _vulkanBackend.GetBufferObj().CreateSSBOBuffer(baseMaterialsBuffersSize);

	// Temporary initialize 5 point lights, would be enough for now
	PointLight pointLight;
	pointLight.position = glm::vec3(0.0f, 0.0f, 0.0f);
	pointLight.radius = 10.0f;
	pointLight.intenstity = 1.5f;
	_pointLights.push_back(pointLight);
	pointLight.position = glm::vec3(0.0f, 0.0f, -25.0f);
	_pointLights.push_back(pointLight);
	_pointLightsBuffer = _vulkanBackend.GetBufferObj().CreateSSBOBuffer(sizeof(PointLight) * _pointLights.size());
	_vulkanBackend.GetBufferObj().UpdateSSBOBuffer(_pointLights.data(), sizeof(PointLight) * _pointLights.size(), _pointLightsBuffer.index);

	VulkanImage& imageManager = _vulkanBackend.GetImageObj();
	const u32 windowWidth = _vulkanBackend.GetPresentationObj().GetSwapchainDesc().extent.width;
	const u32 windowHeight = _vulkanBackend.GetPresentationObj().GetSwapchainDesc().extent.height;


	// Init G buffer
	ImageSpecification imageSpec;
	imageSpec.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	imageSpec.mipLevels = 1;
	imageSpec.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	imageSpec.extent = { windowWidth, windowHeight, 1 };
	imageSpec.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	_gBuffer.positions = imageManager.CreateEmptyImage(imageSpec);
	_gBuffer.normals = imageManager.CreateEmptyImage(imageSpec);
	imageSpec.format = VK_FORMAT_R8G8B8A8_SRGB;
	_gBuffer.baseColor = imageManager.CreateEmptyImage(imageSpec);
	_gBuffer.metallicRoughness = imageManager.CreateEmptyImage(imageSpec);

}



void SceneRenderer::UpdateBuffers(const Entity& entity)
{
	VulkanBuffer& bufferManager = _vulkanBackend.GetBufferObj();
	VulkanImage& imageManager = _vulkanBackend.GetImageObj();

	MeshComponent* meshComp = entity.GetComponent<MeshComponent>();
	// Check if buffer exist
	if (meshComp != nullptr && bufferManager.GetMeshBuffers(entity.GetID()) == nullptr)
	{
		auto loadingResult = AssetManager::Get()->TryToLoadAndStoreMesh(meshComp->folderName);
		if (!loadingResult.has_value())
			return;

		u32 meshIndex = loadingResult->assetID;
		meshComp->meshIndex = meshIndex;
		if (meshIndex != 0)
		{
			const VertexDescription* desc = AssetManager::Get()->GetVertexDesc(meshIndex);
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
			allMeshMaterials.push_back(AssetManager::Get()->TryToLoadMaterial(imageManager, material));
		}

		const auto materialsStoreResult = AssetManager::Get()->StoreLoadedMaterials(allMeshMaterials);
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
			bufferManager.UpdateUniformBuffer(&uniform, sizeof(UniformData), entity.GetID());
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

		bufferManager.CreateUniformBuffer(static_cast<size_t>(sizeof(UniformData)), entity.GetID());
		bufferManager.UpdateUniformBuffer(&uniform, static_cast<size_t>(sizeof(UniformData)), entity.GetID());
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

void SceneRenderer::Update()
{
	VulkanBuffer& bufferManager = _vulkanBackend.GetBufferObj();
	VulkanFrame& frameManager = _vulkanBackend.GetFrameObj();
	VulkanImage& imageManager = _vulkanBackend.GetImageObj();
	VulkanDescriptor& descriptorManager = _vulkanBackend.GetDescriptorObj();
	VulkanPresentation& presentationManager = _vulkanBackend.GetPresentationObj();


	const std::vector<MaterialTexturesDesc>& allMaterials = AssetManager::Get()->GetAllSceneMaterialsDesc();
	bufferManager.UpdateSSBOBuffer(allMaterials.data(), allMaterials.size() * sizeof(MaterialTexturesDesc), _baseMaterialsSSBO.index);

	for (u32 descInd = 0; descInd < VulkanFrame::FramesInFlight; ++descInd)
	{
		const std::vector<std::shared_ptr<ImageHandle>>& images = imageManager.GetAllLoadedImages(); // TEMPORARY SOLUTION. TO REWORK
		assert(!images.empty() && "Images size is zero");
		for (u32 i = 0; i < images.size(); ++i)
		{
			DescriptorUpdate updateData;
			updateData.set = _baseShadingDescriptorSets[descInd];
			updateData.imgView = images[i]->imageView;
			updateData.dstBinding = 0;
			updateData.dstArrayElem = images[i]->index; // starting from 1, zero is null

			updateData.sampler = _samplerLinear;

			descriptorManager.UpdateBindlessDescriptorSet(updateData);
		}
	}

	_currentColorAttachment = presentationManager.GetSwapchainDesc().images[frameManager.GetCurrentImageIndex()];
	_currentDepthAttachment = _depthAttachments[frameManager.GetCurrentImageIndex()];
	
}

void SceneRenderer::Draw()
{
	VulkanFrame& frameManager = _vulkanBackend.GetFrameObj();
	VulkanBuffer& bufferManager = _vulkanBackend.GetBufferObj();

	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// THIS IS TEMPORARY SOLUTION. TO REWORK!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	_vulkanBackend.GetImageObj().UpdateLayoutsToCopyData();
	_vulkanBackend.GetDescriptorObj().UpdateSets();

	Renderer::BeginRender({ _currentColorAttachment, _currentDepthAttachment });

	for(const auto& command : _drawCommands)
	{
		auto& command = _drawCommands.front();
		command.lightPushConstants.materialAddress = _baseMaterialsSSBO.address;
		command.lightPushConstants.lightsAddress = _pointLightsBuffer.address;
		command.lightPushConstants.pointLightCount = _pointLights.size();
		Renderer::RenderMesh(command);

	}
	Renderer::EndRender();


	_drawCommands.clear();
}

// THIS IS ONLY TEMPORARY SOLUTION. TO REWORK ASSET SYSTEM LATER
// THIS IS ONLY TEMPORARY SOLUTION. TO REWORK ASSET SYSTEM LATER
// THIS IS ONLY TEMPORARY SOLUTION. TO REWORK ASSET SYSTEM LATER
// THIS IS ONLY TEMPORARY SOLUTION. TO REWORK ASSET SYSTEM LATER
// THIS IS ONLY TEMPORARY SOLUTION. TO REWORK ASSET SYSTEM LATER
// THIS IS ONLY TEMPORARY SOLUTION. TO REWORK ASSET SYSTEM LATER
// THIS IS ONLY TEMPORARY SOLUTION. TO REWORK ASSET SYSTEM LATER
// THIS IS ONLY TEMPORARY SOLUTION. TO REWORK ASSET SYSTEM LATER
// THIS IS ONLY TEMPORARY SOLUTION. TO REWORK ASSET SYSTEM LATER
// THIS IS ONLY TEMPORARY SOLUTION. TO REWORK ASSET SYSTEM LATER
// THIS IS ONLY TEMPORARY SOLUTION. TO REWORK ASSET SYSTEM LATER


void SceneRenderer::SubmitEntityToDraw(const Entity& entity)
{
	VulkanBuffer& bufferManager = _vulkanBackend.GetBufferObj();
	VulkanFrame& frameManager = _vulkanBackend.GetFrameObj();

	UpdateBuffers(entity);


	const MeshBuffers* meshBuffers = bufferManager.GetMeshBuffers(entity.GetID());
	assert(meshBuffers && "Mesh buffer for entity is empty");
	const StorageBuffer* uniformBuffer = bufferManager.GetUniformBuffer(entity.GetID());
	assert(meshBuffers && "Uniform buffer for entity is empty");

	const MeshComponent* meshComp = entity.GetComponent<MeshComponent>();
	if (meshComp != nullptr)
	{
		const u32 meshAssetIndex = meshComp->meshIndex;
		if (meshAssetIndex == 0)
			return;

		const VertexDescription* vertexDesc =  AssetManager::Get()->GetVertexDesc(meshAssetIndex);
		if (vertexDesc == nullptr)
			return;

		DrawCommand command;
		command.pipeline = _baseShadingPair.pipeline;
		command.pipelineLayout = _baseShadingPair.pipelineLayout;
		command.descriptorSet = _baseShadingDescriptorSets[frameManager.GetCurrentFrameIndex()].descriptorSet;
		command.lightPushConstants.verticesAddress = meshBuffers->GetDeviceAddress();
		command.lightPushConstants.uniformAddress = uniformBuffer->GetDeviceAddress();
		command.indexCount = vertexDesc->indexCount;
		command.indexBuffer = bufferManager.GetMeshBuffers(entity.GetID())->GetVkIndexBuffer();
		_drawCommands.emplace_back(std::move(command));
	}

}