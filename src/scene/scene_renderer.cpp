#include "../../headers/scene/scene_renderer.h"
#include "../../headers/scene/entity.h"

#include <glm/gtc/matrix_transform.hpp>

SceneRenderer::SceneRenderer(VulkanBase& vulkanBackend, EngineBase& engineBase) : _vulkanBackend{ vulkanBackend }, _engineBase{engineBase}
{
	// Descriptor
	// Create descriptor for every frame
	for (u32 i = 0; i < VulkanFrame::FramesInFlight; ++i)
	{
		constexpr i32 descriptorUsageCount = 3;
		DescriptorSpecification sceneDescSpec{};
		sceneDescSpec.bindings.resize(descriptorUsageCount);
		sceneDescSpec.bindings[0].binding = 0;
		sceneDescSpec.bindings[0].descriptorCount = 1024;
		sceneDescSpec.bindings[0].descriptorType = DescriptorType::COMBINED_IMAGE_SAMPLER;

		sceneDescSpec.bindings[1].binding = 1;
		sceneDescSpec.bindings[1].descriptorCount = 1;
		sceneDescSpec.bindings[1].descriptorType = DescriptorType::STORAGE_IMAGE;
		

		// depth texture binding
		sceneDescSpec.bindings[2].descriptorType = DescriptorType::COMBINED_IMAGE_SAMPLER;
		sceneDescSpec.bindings[2].binding = 2;
		sceneDescSpec.bindings[2].descriptorCount = 1;
		
		_sceneDescriptorSets.emplace_back(_engineBase.GetDescriptorManager().CreateDescriptorSet(sceneDescSpec));
	}



	_depthAttachments.resize(VulkanPresentation::PresentationImagesCount);
	for (u32 i = 0; i < VulkanPresentation::PresentationImagesCount; ++i)
	{
		ImageSpecification spec;
		spec.mipLevels = 1;
		spec.usage  =  ImageUsage::IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT | ImageUsage::IMAGE_USAGE_SAMPLED;
		spec.aspect =  ImageAspect::IMAGE_ASPECT_DEPTH;
		spec.format =  ImageFormat::IMAGE_FORMAT_D32_SFLOAT;
		spec.type	=  ImageType::IMAGE_TYPE_DEPTH_BUFFER;
		spec.extent = ImageExtent3D{ static_cast<u32>(Window::GetWindowWidth()), static_cast<u32>(Window::GetWindowHeight()), 1 };

		_depthAttachments[i] = _engineBase.GetImageManager().CreateImage(spec);
	}

	// Sampler
	SamplerSpecification linearSpec;
	linearSpec.minFiltering = Filter::FILTER_LINEAR;
	linearSpec.magFiltering = Filter::FILTER_LINEAR;
	linearSpec.mipmapFiltering =  SamplerMipMapMode::SAMPLER_MIPMAP_MODE_LINEAR;
	linearSpec.addressMode = SamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
	linearSpec.minLod = 0.0f;
	linearSpec.maxLod = 1000.0f;
	linearSpec.lodBias = 0.0f;


	_samplerLinear = _engineBase.GetImageManager().CreateSampler(linearSpec);

	SamplerSpecification nearestSpec;
	nearestSpec.minFiltering = Filter::FILTER_NEAREST;
	nearestSpec.magFiltering = Filter::FILTER_NEAREST;
	nearestSpec.mipmapFiltering = SamplerMipMapMode::SAMPLER_MIPMAP_MODE_NEAREST;
	nearestSpec.addressMode = SamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
	nearestSpec.minLod = 0.0f;
	nearestSpec.maxLod = 1000.0f;
	nearestSpec.lodBias = 0.0f;

	_samplerNearest = _engineBase.GetImageManager().CreateSampler(nearestSpec);

	// Materials buffer
	constexpr size_t baseMaterialsBuffersSize = 1024 * 4; // 4096 bytes
	_baseMaterialsSSBO = _vulkanBackend.GetBufferObj().CreateSSBOBuffer(baseMaterialsBuffersSize);

	// Temporary initialize 5 point lights, would be enough for now
	PointLight pointLight;
	pointLight.radius = 3.0f;
	pointLight.intenstity = 1.5f;
	pointLight.position = glm::vec3(0.0f, 1.5f, 0.0f);
	_pointLights.push_back(pointLight);
	pointLight.position = glm::vec3(0.0f, 1.5f, 5.0f);
	_pointLights.push_back(pointLight);
	pointLight.position = glm::vec3(0.0f, 1.5f, -5.0f);
	_pointLights.push_back(pointLight);
	pointLight.position = glm::vec3(5.0f, 1.5f, 0.0f);
	_pointLights.push_back(pointLight);
	pointLight.position = glm::vec3(-5.0f, 1.5f, 0.0f);
	_pointLights.push_back(pointLight);

	_pointLightsBuffer = _vulkanBackend.GetBufferObj().CreateSSBOBuffer(sizeof(PointLight) * _pointLights.size());
	_vulkanBackend.GetBufferObj().UpdateSSBOBuffer(_pointLights.data(), sizeof(PointLight) * _pointLights.size(), _pointLightsBuffer.index);


	const u32 windowWidth  = _vulkanBackend.GetPresentationObj().GetSwapchainDesc().extent.width;
	const u32 windowHeight = _vulkanBackend.GetPresentationObj().GetSwapchainDesc().extent.height;
	// Init light indices SSBO
	{
		glm::ivec3& numWorkGroups = _lightCullStructures.numWorkGroups;
		const u32 tileSize = _lightCullStructures.tileSize;
		numWorkGroups.x = std::ceil(windowWidth / tileSize);
		numWorkGroups.y = std::ceil(windowHeight / tileSize);
		numWorkGroups.z = 1;

		u32& tilesPerScreen = _lightCullStructures.tilesPerScreen;
		tilesPerScreen = static_cast<u32>(numWorkGroups.x * numWorkGroups.y * numWorkGroups.z);

		_lightCullStructures.lightIndicesBuffer = _vulkanBackend.GetBufferObj().CreateSSBOBuffer(sizeof(i32) 
			* tilesPerScreen * _lightCullStructures.maxLightsPerCluster);
		// It should be empty. Would be filled later

		// This buffer is needed for global lights counter among all comp. shader invocations
		constexpr u32 globalLightsInitialCount = 0;
		_lightCullStructures.globalLightsCountBuffer = _vulkanBackend.GetBufferObj().CreateUniformBuffer(sizeof(u32));
		_vulkanBackend.GetBufferObj().UpdateUniformBuffer(&globalLightsInitialCount, sizeof(u32), _lightCullStructures.globalLightsCountBuffer.index);
	}

	// Camera buffer
	{
		_viewDataBuffer = _vulkanBackend.GetBufferObj().CreateUniformBuffer(sizeof(ViewData));
	}
	

	const ImageManager& imageManager = _engineBase.GetImageManager();

	// Init G buffer
	{
		ImageSpecification imageSpec;
		imageSpec.usage = ImageUsage::IMAGE_USAGE_COLOR_ATTACHMENT | ImageUsage::IMAGE_USAGE_SAMPLED;
		imageSpec.mipLevels = 1;
		imageSpec.aspect = ImageAspect::IMAGE_ASPECT_COLOR;
		imageSpec.extent = { windowWidth, windowHeight, 1 };
		imageSpec.format = ImageFormat::IMAGE_FORMAT_R16G16B16A16_SFLOAT;
		imageSpec.type = ImageType::IMAGE_TYPE_RENDER_TARGET;

		_gBuffer.positions = imageManager.CreateImage(imageSpec);
		_gBuffer.normals   = imageManager.CreateImage(imageSpec);
		imageSpec.format   = ImageFormat::IMAGE_FORMAT_R8G8B8A8_SRGB;
		_gBuffer.baseColor = imageManager.CreateImage(imageSpec);
		_gBuffer.metallicRoughness = imageManager.CreateImage(imageSpec);
	}

	{
		ImageSpecification lightsGridImage;
		lightsGridImage.usage  = ImageUsage::IMAGE_USAGE_SAMPLED | ImageUsage::IMAGE_USAGE_STORAGE_BIT;
		lightsGridImage.mipLevels = 1;
		lightsGridImage.aspect = ImageAspect::IMAGE_ASPECT_COLOR;
		lightsGridImage.extent = { static_cast<u32>(_lightCullStructures.numWorkGroups.x), 
								   static_cast<u32>(_lightCullStructures.numWorkGroups.y), 
								   static_cast<u32>(_lightCullStructures.numWorkGroups.z) };
		lightsGridImage.format = ImageFormat::IMAGE_FORMAT_R32G32_UINT;
		lightsGridImage.type   = ImageType::IMAGE_TYPE_RENDER_TARGET;

		_lightCullStructures.lightsGrid = imageManager.CreateImage(lightsGridImage);
	}


	auto extractRawPtrsLambda = [&]()
		{
			std::vector<Descriptor*> descriptors;

			for (auto& descriptor : _sceneDescriptorSets)
				descriptors.push_back(descriptor.get());

			return descriptors;
		};
	{
		PipelineSpecification pbrShadingPipeline;
		pbrShadingPipeline.type = PipelineType::GRAPHICS_PIPELINE;
		pbrShadingPipeline.shaderName = "PBR_shading";
		pbrShadingPipeline.cullMode = CullMode::CULL_MODE_BACK;
		pbrShadingPipeline.pushConstantSizeBytes = sizeof(PBRPassPushConst);
		pbrShadingPipeline.descriptorSets = { extractRawPtrsLambda() };
		pbrShadingPipeline.depthCompare = CompareOP::COMPARE_OP_LESS;
		pbrShadingPipeline.depthWriteEnable = true;
		pbrShadingPipeline.depthTestEnable  = true;
		pbrShadingPipeline.colorFormats = { ImageFormat::IMAGE_FORMAT_B8G8R8A8_SRGB };

		// other data is aight
		_pbrShadingPipeline = _engineBase.GetPipelineManager().CreatePipeline(pbrShadingPipeline);
	}

	{
		PipelineSpecification gBufferGraphicsPipeline;
		gBufferGraphicsPipeline.type = PipelineType::GRAPHICS_PIPELINE;
		gBufferGraphicsPipeline.shaderName = "g-pass";
		gBufferGraphicsPipeline.cullMode = CullMode::CULL_MODE_BACK;
		gBufferGraphicsPipeline.pushConstantSizeBytes = sizeof(GBufferPushConst);
		gBufferGraphicsPipeline.descriptorSets = { extractRawPtrsLambda() }; // Now only one descriptor layout, to DO
		gBufferGraphicsPipeline.depthCompare = CompareOP::COMPARE_OP_LESS;
		gBufferGraphicsPipeline.depthWriteEnable = true;
		gBufferGraphicsPipeline.depthTestEnable  = true;
		gBufferGraphicsPipeline.attachmentsCount = 4;
		gBufferGraphicsPipeline.colorFormats = { ImageFormat::IMAGE_FORMAT_R16G16B16A16_SFLOAT, ImageFormat::IMAGE_FORMAT_R16G16B16A16_SFLOAT, 
			ImageFormat::IMAGE_FORMAT_R8G8B8A8_SRGB, ImageFormat::IMAGE_FORMAT_R8G8B8A8_SRGB };

		// other data is aight

		_gBufferPipeline = _engineBase.GetPipelineManager().CreatePipeline(gBufferGraphicsPipeline);
	}

	{
		PipelineSpecification lightCullingComputePipeline;
		lightCullingComputePipeline.type = PipelineType::COMPUTE_PIPELINE;
		lightCullingComputePipeline.shaderName = "light_cull";
		lightCullingComputePipeline.descriptorSets = { extractRawPtrsLambda() };
		lightCullingComputePipeline.pushConstantSizeBytes = sizeof(LightCullPushConst);

		_lightCullStructures.lightCullingPipeline = _engineBase.GetPipelineManager().CreatePipeline(lightCullingComputePipeline);
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
	const ImageManager& imageManager = _engineBase.GetImageManager();

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

		// Update buffer with materials
		const std::vector<MaterialTexturesDesc>& allMaterials = AssetManager::Get()->GetAllSceneMaterialsDesc();
		bufferManager.UpdateSSBOBuffer(allMaterials.data(), allMaterials.size() * sizeof(MaterialTexturesDesc), _baseMaterialsSSBO.index);

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


	_currentColorAttachment = presentationManager.GetSwapchainDesc().images[frameManager.GetCurrentImageIndex()].get();
	_currentDepthAttachment = _depthAttachments[frameManager.GetCurrentImageIndex()].get();

	UpdateDescriptors();
	
	//// Camera data buffer
	ViewData viewData;
	viewData.proj = camera.GetProjectionMatrix();
	viewData.view = camera.GetViewMatrix();
	viewData.inverseProjection = camera.GetInverseProjection();
	viewData.viewProj = camera.GetViewProjectionMatrix();
	viewData.position = camera.GetPosition();
	viewData.nearPlane = camera.GetNearPlane();
	viewData.farPlane = camera.GetFarPlane();
	viewData.viewportExt = { _currentColorAttachment->GetSpecification().extent.x, _currentColorAttachment->GetSpecification().extent.y };

	bufferManager.UpdateUniformBuffer(&viewData, sizeof(ViewData), _viewDataBuffer.index);
}

void SceneRenderer::UpdateDescriptors()
{
	VulkanBuffer& bufferManager = _vulkanBackend.GetBufferObj();
	DescriptorManager& descriptorManager = _engineBase.GetDescriptorManager();

	for (u32 descInd = 0; descInd < VulkanFrame::FramesInFlight; ++descInd)
	{
		u32 availableIndex = 0;
		// Main shading pass
		const auto& allTextures = AssetManager::Get()->GetAllTextures(); // TEMPORARY SOLUTION. TO REWORK
		assert(!allTextures.empty() && "Images size is zero");
		for (u32 i = 0; i < allTextures.size(); ++i)
		{
			_sceneDescriptorSets[descInd]->Write(0, allTextures[i].first,
				DescriptorType::COMBINED_IMAGE_SAMPLER, allTextures[i].second.get(), _samplerLinear.get());
			++availableIndex;
		}

		_gBuffer.posIndex    = availableIndex++;
		_gBuffer.normalIndex = availableIndex++;
		_gBuffer.baseIndex   = availableIndex++;
		_gBuffer.metallicRoughnessIndex = availableIndex++;

		// Write g buffer descriptors
		_sceneDescriptorSets[descInd]->Write(0, _gBuffer.posIndex,    DescriptorType::COMBINED_IMAGE_SAMPLER,    _gBuffer.positions.get(), _samplerLinear.get());
		_sceneDescriptorSets[descInd]->Write(0, _gBuffer.normalIndex, DescriptorType::COMBINED_IMAGE_SAMPLER, _gBuffer.normals.get(),   _samplerLinear.get());
		_sceneDescriptorSets[descInd]->Write(0, _gBuffer.baseIndex,   DescriptorType::COMBINED_IMAGE_SAMPLER,   _gBuffer.baseColor.get(), _samplerLinear.get());
		_sceneDescriptorSets[descInd]->Write(0, _gBuffer.metallicRoughnessIndex, DescriptorType::COMBINED_IMAGE_SAMPLER, 
			_gBuffer.metallicRoughness.get(), _samplerLinear.get());
	

		if (_lightCullStructures.lightsGrid->GetSpecification().layout == ImageLayout::IMAGE_LAYOUT_UNDEFINED)
		{
			PipelineBarrierStorage pipelineBarriers;
			PipelineImageBarrierInfo preComputeLayoutTransition;
			preComputeLayoutTransition.srcStageMask = PipelineStage::TOP_OF_PIPE;
			preComputeLayoutTransition.dstStageMask = PipelineStage::COMPUTE_SHADER;
			preComputeLayoutTransition.srcAccessMask = AccessFlag::NONE;
			preComputeLayoutTransition.dstAccessMask = AccessFlag::SHADER_WRITE;
			preComputeLayoutTransition.newLayout = ImageLayout::IMAGE_LAYOUT_GENERAL;
			preComputeLayoutTransition.image = _lightCullStructures.lightsGrid.get();

			pipelineBarriers.imageBarriers.push_back(preComputeLayoutTransition);

			Renderer::ExecuteBarriers(pipelineBarriers);
		}

		// Add light's grid to get data about lights
		_sceneDescriptorSets[descInd]->Write(1, 0, DescriptorType::STORAGE_IMAGE, _lightCullStructures.lightsGrid.get(), _samplerLinear.get());


		// depth texture
		_sceneDescriptorSets[descInd]->Write(2, 0, DescriptorType::COMBINED_IMAGE_SAMPLER, _currentDepthAttachment, _samplerNearest.get());
	}
}

void SceneRenderer::Draw()
{
	VulkanFrame& frameManager = _vulkanBackend.GetFrameObj();
	VulkanBuffer& bufferManager = _vulkanBackend.GetBufferObj();

	PipelineBarrierStorage pipelineBarriers;

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// !!!!!!!!!!!!!!!!!!!!!!!!TO REWORK PUSH CONSTANTS TO ABSTRACT VULKAN!!!!!!!!!!!!!!!!!!!!!!!!
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// Update buff
	constexpr u32 clearGlobalLightCount = 0;
	_vulkanBackend.GetBufferObj().UpdateUniformBuffer(&clearGlobalLightCount, sizeof(u32), _lightCullStructures.globalLightsCountBuffer.index);

	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//!!!!!!!!!!!!!!!!!!!!! TO DO: VULKAN ABSTRACTION !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	PipelineImageBarrierInfo preGbufferLayoutTransition;
	preGbufferLayoutTransition.srcStageMask =  PipelineStage::FRAGMENT_SHADER;
	preGbufferLayoutTransition.dstStageMask =  PipelineStage::COLOR_ATTACHMENT_OUTPUT;
	preGbufferLayoutTransition.srcAccessMask = AccessFlag::SHADER_READ;
	preGbufferLayoutTransition.dstAccessMask = AccessFlag::COLOR_ATTACHMENT_WRITE;
	preGbufferLayoutTransition.image = _gBuffer.positions.get();
	preGbufferLayoutTransition.newLayout = ImageLayout::IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	pipelineBarriers.imageBarriers.push_back(preGbufferLayoutTransition);
	preGbufferLayoutTransition.image = _gBuffer.normals.get();
	pipelineBarriers.imageBarriers.push_back(preGbufferLayoutTransition);
	preGbufferLayoutTransition.image = _gBuffer.baseColor.get();
	pipelineBarriers.imageBarriers.push_back(preGbufferLayoutTransition);
	preGbufferLayoutTransition.image = _gBuffer.metallicRoughness.get();
	pipelineBarriers.imageBarriers.push_back(preGbufferLayoutTransition);



	Renderer::ExecuteBarriers(pipelineBarriers);

	// GEOMETRY PASS
 	Renderer::BeginRender({ _gBuffer.positions.get(), _gBuffer.normals.get(), 
		_gBuffer.baseColor.get(), _gBuffer.metallicRoughness.get(), _currentDepthAttachment},
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
			command.descriptor = _sceneDescriptorSets[frameManager.GetCurrentFrameIndex()].get();
			command.pushConstants = pushConstants;
			command.indexCount = vertexDesc->indexCount;
			command.indexBuffer = bufferManager.GetMeshBuffers(entity->GetID())->GetVkIndexBuffer();
			command.pipeline = _gBufferPipeline.get();
			Renderer::RenderMesh(command);
		}
	}
	Renderer::EndRender();

	////// Firsly dispatch comp. shader to fill lights buffer
	PipelineImageBarrierInfo preComputeLayoutTransition;
	preComputeLayoutTransition.srcStageMask = PipelineStage::FRAGMENT_SHADER;
	preComputeLayoutTransition.dstStageMask = PipelineStage::COMPUTE_SHADER;
	preComputeLayoutTransition.srcAccessMask = AccessFlag::SHADER_READ;
	preComputeLayoutTransition.dstAccessMask = AccessFlag::SHADER_WRITE;
	preComputeLayoutTransition.image = _lightCullStructures.lightsGrid.get();
	preComputeLayoutTransition.newLayout = ImageLayout::IMAGE_LAYOUT_GENERAL; // STORAGE IMAGE SHOULD BE IN THE GENERAL LAYOUT
	pipelineBarriers.imageBarriers.push_back(preComputeLayoutTransition);

	// depth image
	preComputeLayoutTransition.srcStageMask = PipelineStage::LATE_FRAGMENT_TESTS;
	preComputeLayoutTransition.dstStageMask = PipelineStage::COMPUTE_SHADER;
	preComputeLayoutTransition.srcAccessMask = AccessFlag::DEPTH_STENCIL_ATTACHMENT_WRITE;
	preComputeLayoutTransition.dstAccessMask = AccessFlag::SHADER_READ;
	preComputeLayoutTransition.image = _currentDepthAttachment;
	preComputeLayoutTransition.newLayout = ImageLayout::IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	preComputeLayoutTransition.aspect = ImageAspect::IMAGE_ASPECT_DEPTH;
	pipelineBarriers.imageBarriers.push_back(preComputeLayoutTransition);




	LightCullPushConst* lightCullPushConst = new LightCullPushConst;
	lightCullPushConst->lightsListAddress = _pointLightsBuffer.address;
	lightCullPushConst->lightsCount = _pointLights.size();
	lightCullPushConst->lightsIndicesAddress = _lightCullStructures.lightIndicesBuffer.address;
	lightCullPushConst->cameraDataAddress = _viewDataBuffer.address;
	lightCullPushConst->globalLightsCounterAddress = _lightCullStructures.globalLightsCountBuffer.address; // MAKE IT ZERO AFTER DISPATCH
	lightCullPushConst->maxLightsPerCluster = _lightCullStructures.maxLightsPerCluster;
	lightCullPushConst->tileSize = _lightCullStructures.tileSize;

	PushConsts lightCullPushConstants;
	lightCullPushConstants.data = (byte*)lightCullPushConst;
	lightCullPushConstants.size = sizeof(LightCullPushConst);



	DispatchCommand lightCullDispatch;
	lightCullDispatch.pipeline = _lightCullStructures.lightCullingPipeline.get();
	lightCullDispatch.descriptor = _sceneDescriptorSets[frameManager.GetCurrentFrameIndex()].get();
	lightCullDispatch.pushConstants = lightCullPushConstants;
	lightCullDispatch.numWorkgroups = { _lightCullStructures.numWorkGroups };


	Renderer::ExecuteBarriers(pipelineBarriers);

	Renderer::DispatchCompute(lightCullDispatch);


	// Main shading pass
	PipelineImageBarrierInfo imageGBufferLightPassBarrier;
	imageGBufferLightPassBarrier.srcStageMask  = PipelineStage::COLOR_ATTACHMENT_OUTPUT;
	imageGBufferLightPassBarrier.dstStageMask  = PipelineStage::FRAGMENT_SHADER;
	imageGBufferLightPassBarrier.srcAccessMask = AccessFlag::COLOR_ATTACHMENT_WRITE;
	imageGBufferLightPassBarrier.dstAccessMask = AccessFlag::SHADER_READ;
	imageGBufferLightPassBarrier.image = _gBuffer.positions.get();
	imageGBufferLightPassBarrier.newLayout = ImageLayout::IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	pipelineBarriers.imageBarriers.push_back(imageGBufferLightPassBarrier);
	imageGBufferLightPassBarrier.image = _gBuffer.normals.get();
	pipelineBarriers.imageBarriers.push_back(imageGBufferLightPassBarrier);
	imageGBufferLightPassBarrier.image = _gBuffer.baseColor.get();
	pipelineBarriers.imageBarriers.push_back(imageGBufferLightPassBarrier);
	imageGBufferLightPassBarrier.image = _gBuffer.metallicRoughness.get();
	pipelineBarriers.imageBarriers.push_back(imageGBufferLightPassBarrier);

	imageGBufferLightPassBarrier.srcStageMask = PipelineStage::COMPUTE_SHADER;
	imageGBufferLightPassBarrier.dstStageMask = PipelineStage::FRAGMENT_SHADER;
	imageGBufferLightPassBarrier.srcAccessMask = AccessFlag::SHADER_WRITE;
	imageGBufferLightPassBarrier.dstAccessMask = AccessFlag::SHADER_READ;
	imageGBufferLightPassBarrier.newLayout = ImageLayout::IMAGE_LAYOUT_GENERAL; // STORAGE IMAGE SHOULD BE IN THE GENERAL LAYOUT
	imageGBufferLightPassBarrier.image = _lightCullStructures.lightsGrid.get();


	
	pipelineBarriers.imageBarriers.push_back(imageGBufferLightPassBarrier);


	Renderer::ExecuteBarriers(pipelineBarriers);


	PBRPassPushConst* pbrPassPushConst = new PBRPassPushConst;
	pbrPassPushConst->lightAddress = _pointLightsBuffer.address;
	pbrPassPushConst->lightsIndicesAddress = _lightCullStructures.lightIndicesBuffer.address;
	pbrPassPushConst->cameraDataAddress = _viewDataBuffer.address;
	pbrPassPushConst->pointLightsCount = _pointLights.size();
	pbrPassPushConst->positionTextureIdx = _gBuffer.posIndex;
	pbrPassPushConst->normalsTextureIdx = _gBuffer.normalIndex;
	pbrPassPushConst->baseColorTextureIdx = _gBuffer.baseIndex;
	pbrPassPushConst->metallicRoughnessTextureIdx = _gBuffer.metallicRoughnessIndex;
	pbrPassPushConst->tileSize = _lightCullStructures.tileSize;

	PushConsts pushConstants;
	pushConstants.data = (byte*)pbrPassPushConst;
	pushConstants.size = sizeof(PBRPassPushConst);

	// LIGHT PASS
	DrawCommand quadDrawCommand;
	quadDrawCommand.pipeline   = _pbrShadingPipeline.get();
	quadDrawCommand.descriptor = _sceneDescriptorSets[frameManager.GetCurrentFrameIndex()].get();
	quadDrawCommand.pushConstants = pushConstants;

	Renderer::BeginRender({ _currentColorAttachment }, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
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