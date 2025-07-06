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
		{
			DescriptorInfo baseShadingDescInfo{};
			baseShadingDescInfo.bindings.resize(descriptorUsageCount);
			baseShadingDescInfo.bindings[0].binding = 0;
			baseShadingDescInfo.bindings[0].descriptorCount = 1024;
			baseShadingDescInfo.bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			baseShadingDescInfo.bindings[0].stageFlags = VK_SHADER_STAGE_ALL;
			_baseShadingDescriptorSets.emplace_back(vulkanBackend.GetDescriptorObj().CreateDescSet(baseShadingDescInfo));
		}
		
		{
			DescriptorInfo gBuffdescInfo{};
			gBuffdescInfo.bindings.resize(descriptorUsageCount);
			gBuffdescInfo.bindings[0].binding = 0;
			gBuffdescInfo.bindings[0].descriptorCount = 1024;
			gBuffdescInfo.bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			gBuffdescInfo.bindings[0].stageFlags = VK_SHADER_STAGE_ALL;
			_gBuffDescriptorSets.emplace_back(vulkanBackend.GetDescriptorObj().CreateDescSet(gBuffdescInfo));
		}
		
		{
			DescriptorInfo lightCullCompInfo{};
			lightCullCompInfo.bindings.resize(descriptorUsageCount);
			lightCullCompInfo.bindings[0].binding = 0;
			lightCullCompInfo.bindings[0].descriptorCount = 1;
			lightCullCompInfo.bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			lightCullCompInfo.bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			_lightCullStructures.lightCullingDescriptorSets.emplace_back(vulkanBackend.GetDescriptorObj().CreateDescSet(lightCullCompInfo));
		}
	}

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


	const u32 windowWidth = _vulkanBackend.GetPresentationObj().GetSwapchainDesc().extent.width;
	const u32 windowHeight = _vulkanBackend.GetPresentationObj().GetSwapchainDesc().extent.height;
	// Init light indices SSBO
	{
		glm::ivec3& numWorkGroups = _lightCullStructures.numWorkGroups;
		numWorkGroups.x = std::ceil(windowWidth / 16);
		numWorkGroups.y = std::ceil(windowHeight / 16);
		numWorkGroups.z = 1;

		u32& tilesPerScreen = _lightCullStructures.tilesPerScreen;
		tilesPerScreen = static_cast<u32>(numWorkGroups.x * numWorkGroups.y * numWorkGroups.z);

		_lightCullStructures.lightIndicesBuffer = _vulkanBackend.GetBufferObj().CreateSSBOBuffer(sizeof(i32) 
			* tilesPerScreen * _lightCullStructures.maxLightsPerCluster);
		// It should be empty. Would be filled later
	}

	// Camera buffer
	{
		_viewDataBuffer = _vulkanBackend.GetBufferObj().CreateUniformBuffer(sizeof(ViewData));
	}
	

	VulkanImage& imageManager = _vulkanBackend.GetImageObj();

	// Init G buffer
	{
		ImageSpecification imageSpec;
		imageSpec.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
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

	{
		ImageSpecification lightsGridImage;
		lightsGridImage.usage = VK_IMAGE_USAGE_STORAGE_BIT;
		lightsGridImage.mipLevels = 1;
		lightsGridImage.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		lightsGridImage.extent = { windowWidth, windowHeight, 1 };
		lightsGridImage.format = VK_FORMAT_R32G32_UINT;
		_lightCullStructures.lightsGrid = imageManager.CreateEmptyImage(lightsGridImage);
	}


	{
		GraphicsPipeline baseShadingPipeline;
		baseShadingPipeline.shaderName = "shading";
		baseShadingPipeline.cullMode = VK_CULL_MODE_BACK_BIT;
		baseShadingPipeline.pushConstantSizeBytes = sizeof(LightPassPushConst);
		baseShadingPipeline.descriptorLayouts = { _vulkanBackend.GetDescriptorObj().GetBindlessDescriptorSetLayout() }; // Now only one descriptor layout, to DO
		baseShadingPipeline.depthCompare = VK_COMPARE_OP_LESS;
		baseShadingPipeline.depthWriteEnable = VK_TRUE;
		baseShadingPipeline.depthTestEnable = VK_TRUE;
		baseShadingPipeline.colorFormats = { VulkanPresentation::ColorFormat.format };

		// other data is aight
		_baseShadingPipeline = _vulkanBackend.GetPipelineObj().CreatePipeline(baseShadingPipeline);
	}

	{
		GraphicsPipeline gBufferGraphicsPipeline;
		gBufferGraphicsPipeline.shaderName = "g-pass";
		gBufferGraphicsPipeline.cullMode = VK_CULL_MODE_BACK_BIT;
		gBufferGraphicsPipeline.pushConstantSizeBytes = sizeof(GBufferPushConst);
		gBufferGraphicsPipeline.descriptorLayouts = { _vulkanBackend.GetDescriptorObj().GetBindlessDescriptorSetLayout() }; // Now only one descriptor layout, to DO
		gBufferGraphicsPipeline.depthCompare = VK_COMPARE_OP_LESS;
		gBufferGraphicsPipeline.depthWriteEnable = VK_TRUE;
		gBufferGraphicsPipeline.depthTestEnable = VK_TRUE;
		gBufferGraphicsPipeline.attachmentsCount = 4;
		gBufferGraphicsPipeline.colorFormats = { VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB };

		// other data is aight

		_gBufferPipeline = _vulkanBackend.GetPipelineObj().CreatePipeline(gBufferGraphicsPipeline);
	}


	{
		ComputePipeline lightCullingComputePipeline;
		lightCullingComputePipeline.shaderName = "light-cull";
		lightCullingComputePipeline.descriptorLayouts = { _vulkanBackend.GetDescriptorObj().GetBindlessDescriptorSetLayout() };
		lightCullingComputePipeline.pushConstantSizeBytes = sizeof(LightCullPushConst);

		_lightCullStructures.lightCullingPipeline = _vulkanBackend.GetPipelineObj().CreatePipeline(lightCullingComputePipeline);
	}


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
				vertexDesc.bufferMemoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

				MeshIndexBufferCreateDesc indexDesc;
				indexDesc.indicesPtr = desc->indicesPtr;
				indexDesc.elementsCount = desc->indexCount;
				indexDesc.bufferUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
				indexDesc.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				indexDesc.bufferMemoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

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


	if (_baseTransformationBuffer.index != -1)
	{
		const TranslationComponent* transComp = entity.GetComponent<TranslationComponent>();
		glm::mat4 model = transComp ? GenerateModelMatrix(*transComp) : glm::mat4(1.0f);
		EntityUniformData uniform{};
		uniform.model = model;
		bufferManager.UpdateUniformBuffer(&uniform, sizeof(EntityUniformData), _baseTransformationBuffer.index);
		
	}
	else // create
	{
		EntityUniformData uniform{};
		const TranslationComponent* transComp = entity.GetComponent<TranslationComponent>();
		glm::mat4 model = transComp ? GenerateModelMatrix(*transComp) : glm::mat4(1.0f);
		uniform.model = model;

		_baseTransformationBuffer = bufferManager.CreateUniformBuffer(static_cast<size_t>(sizeof(EntityUniformData)));
		bufferManager.UpdateUniformBuffer(&uniform, static_cast<size_t>(sizeof(EntityUniformData)), _baseTransformationBuffer.index);
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

void SceneRenderer::Update(const Camera& camera)
{
	VulkanFrame& frameManager = _vulkanBackend.GetFrameObj();
	VulkanPresentation& presentationManager = _vulkanBackend.GetPresentationObj();
	VulkanBuffer& bufferManager = _vulkanBackend.GetBufferObj();


	_currentColorAttachment = presentationManager.GetSwapchainDesc().images[frameManager.GetCurrentImageIndex()];
	_currentDepthAttachment = _depthAttachments[frameManager.GetCurrentImageIndex()];

	UpdateDescriptors();


	const std::vector<MaterialTexturesDesc>& allMaterials = AssetManager::Get()->GetAllSceneMaterialsDesc();
	bufferManager.UpdateSSBOBuffer(allMaterials.data(), allMaterials.size() * sizeof(MaterialTexturesDesc), _baseMaterialsSSBO.index);
	
	//// Camera data buffer
	ViewData viewData;
	viewData.proj = camera.GetProjectionMatrix();
	viewData.view = camera.GetViewMatrix();
	viewData.inverseProjection = camera.GetInverseProjection();
	viewData.viewProj = camera.GetViewProjectionMatrix();
	viewData.position = camera.GetPosition();
	viewData.nearPlane = camera.GetNearPlane();
	viewData.farPlane = camera.GetFarPlane();
	viewData.viewportExt = { _currentColorAttachment->extent.width, _currentColorAttachment->extent.height };

	bufferManager.UpdateUniformBuffer(&viewData, sizeof(ViewData), _viewDataBuffer.index);
}

void SceneRenderer::UpdateDescriptors()
{
	VulkanBuffer& bufferManager = _vulkanBackend.GetBufferObj();
	VulkanImage& imageManager = _vulkanBackend.GetImageObj();
	VulkanDescriptor& descriptorManager = _vulkanBackend.GetDescriptorObj();

	for (u32 descInd = 0; descInd < VulkanFrame::FramesInFlight; ++descInd)
	{
		const std::vector<std::shared_ptr<ImageHandle>>& images = imageManager.GetAllLoadedImages(); // TEMPORARY SOLUTION. TO REWORK
		assert(!images.empty() && "Images size is zero");
		for (u32 i = 0; i < images.size(); ++i)
		{
			// G buffer desc set
			DescriptorUpdate updateData;
			updateData.set = _gBuffDescriptorSets[descInd];
			updateData.imgView = images[i]->imageView;
			updateData.dstBinding = 0;
			updateData.dstArrayElem = images[i]->index; // starting from 1, zero is null

			updateData.sampler = _samplerLinear;

			descriptorManager.UpdateBindlessDescriptorSet(updateData);
		}

		// Shading desc set
		DescriptorUpdate shadingDescUpdate;
		shadingDescUpdate.set = _baseShadingDescriptorSets[descInd];
		shadingDescUpdate.imgView = _gBuffer.positions->imageView;
		shadingDescUpdate.dstBinding = 0;
		shadingDescUpdate.dstArrayElem = _gBuffer.positions->index;
		shadingDescUpdate.sampler = _samplerLinear;
		descriptorManager.UpdateBindlessDescriptorSet(shadingDescUpdate);
		shadingDescUpdate.imgView = _gBuffer.normals->imageView;
		shadingDescUpdate.dstArrayElem = _gBuffer.normals->index;
		descriptorManager.UpdateBindlessDescriptorSet(shadingDescUpdate);
		shadingDescUpdate.imgView = _gBuffer.baseColor->imageView;
		shadingDescUpdate.dstArrayElem = _gBuffer.baseColor->index;
		descriptorManager.UpdateBindlessDescriptorSet(shadingDescUpdate);
		shadingDescUpdate.imgView = _gBuffer.metallicRoughness->imageView;
		shadingDescUpdate.dstArrayElem = _gBuffer.metallicRoughness->index;
		descriptorManager.UpdateBindlessDescriptorSet(shadingDescUpdate);


		DescriptorUpdate lightCullDescUpdate;
		lightCullDescUpdate.set = _lightCullStructures.lightCullingDescriptorSets[descInd];
		lightCullDescUpdate.dstBinding = 0; // Only those are binded through desc set, so 0
		lightCullDescUpdate.dstArrayElem = 0; // Only those are binded through desc set, so 0 
		lightCullDescUpdate.imgView = _lightCullStructures.lightsGrid->imageView;
		lightCullDescUpdate.sampler = _samplerLinear;
	}
}

void SceneRenderer::Draw()
{
	VulkanFrame& frameManager = _vulkanBackend.GetFrameObj();
	VulkanBuffer& bufferManager = _vulkanBackend.GetBufferObj();

	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//!!!!!!!!!!!!!!!!!! THIS IS TEMPORARY SOLUTION. TO REWORK !!!!!!!!!!!!!!!!!!!!!!
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	_vulkanBackend.GetImageObj().UpdateLayoutsToCopyData();
	PipelineBarrierStorage pipelineBarriers;

	//// Firsly dispatch comp. shader to fill lights buffer
	LightCullPushConst* lightCullPushConst = new LightCullPushConst;
	lightCullPushConst->lightsListAddress = _pointLightsBuffer.address;
	lightCullPushConst->lightsCount = _pointLights.size();
	lightCullPushConst->lightsIndicesAddress = _lightCullStructures.lightIndicesBuffer.address;
	lightCullPushConst->cameraDataAddress = _viewDataBuffer.address;
	lightCullPushConst->maxLightsPerCluster = _lightCullStructures.maxLightsPerCluster;

	//PushConsts lightCullPushConstants;
	//lightCullPushConstants.data = (byte*)lightCullPushConst;
	//lightCullPushConstants.size = sizeof(LightCullPushConst);



	//DispatchCommand lightCullDispatch;
	//lightCullDispatch.pipeline = _lightCullStructures.lightCullingPipeline.pipeline;
	//lightCullDispatch.pipelineLayout = _lightCullStructures.lightCullingPipeline.pipelineLayout;
	//lightCullDispatch.descriptorSet = _lightCullStructures.lightCullingDescriptorSets[frameManager.GetCurrentFrameIndex()].descriptorSet;
	//lightCullDispatch.pushConstants = lightCullPushConstants;
	//lightCullDispatch.numWorkgroups = { _lightCullStructures.numWorkGroups };


	//Renderer::DispatchCompute(lightCullDispatch);









	
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//!!!!!!!!!!!!!!!!!!!!! TO DO: VULKAN ABSTRACTION !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	PipelineImageBarrierInfo preGbufferLayoutTransition;
	preGbufferLayoutTransition.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	preGbufferLayoutTransition.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	preGbufferLayoutTransition.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
	preGbufferLayoutTransition.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	preGbufferLayoutTransition.imgHandle = _gBuffer.positions;
	preGbufferLayoutTransition.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	pipelineBarriers.imageBarriers.push_back(preGbufferLayoutTransition);
	preGbufferLayoutTransition.imgHandle = _gBuffer.normals;
	pipelineBarriers.imageBarriers.push_back(preGbufferLayoutTransition);
	preGbufferLayoutTransition.imgHandle = _gBuffer.baseColor;
	pipelineBarriers.imageBarriers.push_back(preGbufferLayoutTransition);
	preGbufferLayoutTransition.imgHandle = _gBuffer.metallicRoughness;
	pipelineBarriers.imageBarriers.push_back(preGbufferLayoutTransition);

	Renderer::ExecuteBarriers(pipelineBarriers);

	// GEOMETRY PASS
	Renderer::BeginRender({ _gBuffer.positions, _gBuffer.normals, _gBuffer.baseColor, _gBuffer.metallicRoughness, _currentDepthAttachment }, 
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

	for (auto entity : _drawCommands)
	{
		if (entity == nullptr)
			continue;

		const MeshBuffers* meshBuffers = bufferManager.GetMeshBuffers(entity->GetID());
		assert(meshBuffers && "Mesh buffer for entity is empty");

		const MeshComponent* meshComp = entity->GetComponent<MeshComponent>();
		if (meshComp != nullptr)
		{
			const u32 meshAssetIndex = meshComp->meshIndex;
			if (meshAssetIndex == 0)
				continue;

			const VertexDescription* vertexDesc = AssetManager::Get()->GetVertexDesc(meshAssetIndex);
			if (vertexDesc == nullptr)
				continue;

			GBufferPushConst* gBuffPushConst = new GBufferPushConst;
			gBuffPushConst->vertexAddress = meshBuffers->GetDeviceAddress();
			gBuffPushConst->entityUniformAddress = _baseTransformationBuffer.address;
			gBuffPushConst->materialAddress = _baseMaterialsSSBO.address;
			gBuffPushConst->cameraDataAddress = _viewDataBuffer.address;
						   
			PushConsts pushConstants;
			pushConstants.data = (byte*)gBuffPushConst;
			pushConstants.size = sizeof(GBufferPushConst);

			DrawCommand command;
			command.descriptorSet = _gBuffDescriptorSets[frameManager.GetCurrentFrameIndex()].descriptorSet;
			command.pushConstants = pushConstants;
			command.indexCount = vertexDesc->indexCount;
			command.indexBuffer = bufferManager.GetMeshBuffers(entity->GetID())->GetVkIndexBuffer();
			command.pipeline = _gBufferPipeline.pipeline;
			command.pipelineLayout = _gBufferPipeline.pipelineLayout;
			Renderer::RenderMesh(command);
		}
	}
	Renderer::EndRender();

	PipelineImageBarrierInfo imageGBufferLightPassBarrier;
	imageGBufferLightPassBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	imageGBufferLightPassBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	imageGBufferLightPassBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	imageGBufferLightPassBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
	imageGBufferLightPassBarrier.imgHandle = _gBuffer.positions;
	imageGBufferLightPassBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	pipelineBarriers.imageBarriers.push_back(imageGBufferLightPassBarrier);
	imageGBufferLightPassBarrier.imgHandle = _gBuffer.normals;
	pipelineBarriers.imageBarriers.push_back(imageGBufferLightPassBarrier);
	imageGBufferLightPassBarrier.imgHandle = _gBuffer.baseColor;
	pipelineBarriers.imageBarriers.push_back(imageGBufferLightPassBarrier);
	imageGBufferLightPassBarrier.imgHandle = _gBuffer.metallicRoughness;
	pipelineBarriers.imageBarriers.push_back(imageGBufferLightPassBarrier);

	Renderer::ExecuteBarriers(pipelineBarriers);


	LightPassPushConst* lightPassPushConst = new LightPassPushConst;
	lightPassPushConst->lightAddress = _pointLightsBuffer.address;
	lightPassPushConst->pointLightsCount = _pointLights.size();
	lightPassPushConst->positionTextureIdx = _gBuffer.positions->index;
	lightPassPushConst->normalsTextureIdx = _gBuffer.normals->index;
	lightPassPushConst->baseColorTextureIdx = _gBuffer.baseColor->index;
	lightPassPushConst->metallicRoughnessTextureIdx = _gBuffer.metallicRoughness->index;

	PushConsts pushConstants;
	pushConstants.data = (byte*)lightPassPushConst;
	pushConstants.size = sizeof(LightPassPushConst);

	// LIGHT PASS
	DrawCommand quadDrawCommand;
	quadDrawCommand.pipeline = _baseShadingPipeline.pipeline;
	quadDrawCommand.pipelineLayout = _baseShadingPipeline.pipelineLayout;
	quadDrawCommand.descriptorSet = _baseShadingDescriptorSets[frameManager.GetCurrentFrameIndex()].descriptorSet;
	quadDrawCommand.pushConstants = pushConstants;

	Renderer::BeginRender({ _currentColorAttachment, _currentDepthAttachment }, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	Renderer::RenderQuad(quadDrawCommand);
	Renderer::EndRender();

	_drawCommands.clear();
}

void SceneRenderer::SubmitEntityToDraw(const Entity& entity)
{
	VulkanBuffer& bufferManager = _vulkanBackend.GetBufferObj();
	VulkanFrame& frameManager = _vulkanBackend.GetFrameObj();

	UpdateBuffers(entity);

	_drawCommands.push_back(&entity);


}