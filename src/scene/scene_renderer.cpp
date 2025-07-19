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

	{
		// Materials buffer
		constexpr size_t baseMaterialsBuffersSize = 1024 * 4; // 4096 bytes
		BufferSpecification spec{};
		spec.usage = BufferUsage::STORAGE_BUFFER | BufferUsage::TRANSFER_DST | BufferUsage::SHADER_DEVICE_ADDRESS;
		spec.memoryUsage = MemoryUsage::AUTO_PREFER_DEVICE;
		spec.memoryProp = MemoryProperty::DEVICE_LOCAL;
		spec.sharingMode = SharingMode::SHARING_EXCLUSIVE;
		spec.size = baseMaterialsBuffersSize;
		_baseMaterialsSSBO = _engineBase.GetBufferManager().CreateBuffer(spec);
	}

	// Temporary initialize 5 point lights, would be enough for now
	{
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

		BufferSpecification spec{};
		spec.usage = BufferUsage::STORAGE_BUFFER |  BufferUsage::TRANSFER_DST | BufferUsage::SHADER_DEVICE_ADDRESS;
		spec.memoryUsage = MemoryUsage::AUTO_PREFER_DEVICE;
		spec.memoryProp = MemoryProperty::DEVICE_LOCAL;
		spec.sharingMode = SharingMode::SHARING_EXCLUSIVE;
		spec.size = sizeof(PointLight) * _pointLights.size();

		_pointLightsBuffer = _engineBase.GetBufferManager().CreateBuffer(spec);
	}

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

		BufferSpecification spec{};
		spec.usage = BufferUsage::STORAGE_BUFFER | BufferUsage::TRANSFER_DST | BufferUsage::SHADER_DEVICE_ADDRESS;
		spec.memoryUsage = MemoryUsage::AUTO_PREFER_DEVICE;
		spec.memoryProp = MemoryProperty::DEVICE_LOCAL;
		spec.sharingMode = SharingMode::SHARING_EXCLUSIVE;
		spec.size = sizeof(i32) * tilesPerScreen * _lightCullStructures.maxLightsPerCluster;


		// It should be empty. Would be filled later
		_lightCullStructures.lightIndicesBuffer = _engineBase.GetBufferManager().CreateBuffer(spec);
	}

	// Camera buffer
	{
		BufferSpecification spec{};
		spec.usage = BufferUsage::STORAGE_BUFFER | BufferUsage::TRANSFER_DST | BufferUsage::SHADER_DEVICE_ADDRESS;
		spec.memoryUsage = MemoryUsage::AUTO_PREFER_DEVICE;
		spec.memoryProp = MemoryProperty::DEVICE_LOCAL;
		spec.sharingMode = SharingMode::SHARING_EXCLUSIVE;
		spec.size = sizeof(ViewData);

		_viewDataBuffer = _engineBase.GetBufferManager().CreateBuffer(spec);
	}
	
	// Buffer for indirect draws
	{
		BufferSpecification spec{};
		spec.usage = BufferUsage::INDIRECT_BUFFER | BufferUsage::TRANSFER_DST | BufferUsage::SHADER_DEVICE_ADDRESS;
		spec.memoryUsage = MemoryUsage::AUTO_PREFER_DEVICE;
		spec.memoryProp = MemoryProperty::DEVICE_LOCAL;
		spec.sharingMode = SharingMode::SHARING_EXCLUSIVE;
		spec.size = 1024 * sizeof(DrawIndexedIndirectCommand); // +1 for count buffer

		
		_indirectBuffer.opaqueBuffer = _engineBase.GetBufferManager().CreateBuffer(spec);
		_indirectBuffer.maskBuffer = _engineBase.GetBufferManager().CreateBuffer(spec);

		// count buffer is at the end
		_indirectBuffer.countBufferOffset = spec.size - sizeof(DrawIndexedIndirectCommand);
	}
	
	// All scene meshes buffer
	{
		constexpr u32 size = sizeof(Vertex) * 1024 * 1024; // about 1 million vertices
		BufferSpecification spec{};
		spec.usage = BufferUsage::VERTEX_BUFFER | BufferUsage::TRANSFER_DST | BufferUsage::SHADER_DEVICE_ADDRESS;
		spec.memoryUsage = MemoryUsage::AUTO_PREFER_DEVICE;
		spec.memoryProp = MemoryProperty::DEVICE_LOCAL;
		spec.sharingMode = SharingMode::SHARING_EXCLUSIVE;
		spec.size = size;

		_meshDeviceBuffer.vertexBuffer = _engineBase.GetBufferManager().CreateBuffer(spec);

		spec.usage = BufferUsage::INDEX_BUFFER | BufferUsage::TRANSFER_DST | BufferUsage::SHADER_DEVICE_ADDRESS;
		_meshDeviceBuffer.indexBuffer = _engineBase.GetBufferManager().CreateBuffer(spec);



		// common buffer with transformations, materials and their indices
		spec.usage = BufferUsage::STORAGE_BUFFER | BufferUsage::TRANSFER_DST | BufferUsage::SHADER_DEVICE_ADDRESS;
		spec.size = sizeof(CommonIndirectData) * 1024; // 1024 materials, transformations etc
		_indirectBuffer.commonDataBuffer = _engineBase.GetBufferManager().CreateBuffer(spec);

		spec.size = sizeof(CommonIndirectIndices) * 1024; // 1024 indices of materials, transformations etc
		_indirectBuffer.commonIndicesBuffer = _engineBase.GetBufferManager().CreateBuffer(spec);
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
		pbrShadingPipeline.shaderName = "PBR-shading";
		pbrShadingPipeline.entryPoints = {"VertexMain", "FragmentMain"};
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
		gBufferGraphicsPipeline.entryPoints = { "VertexMain", "FragmentMain" };
		gBufferGraphicsPipeline.pushConstantSizeBytes = sizeof(IndirectPushConst);
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
		lightCullingComputePipeline.shaderName = "light-cull";
		lightCullingComputePipeline.entryPoints = { "ComputeMain" };
		lightCullingComputePipeline.descriptorSets = { extractRawPtrsLambda() };
		lightCullingComputePipeline.pushConstantSizeBytes = sizeof(LightCullPushConst);

		_lightCullStructures.lightCullingPipeline = _engineBase.GetPipelineManager().CreatePipeline(lightCullingComputePipeline);
	}


}


void SceneRenderer::Update(const Camera& camera)
{
	VulkanFrame& frameManager = _vulkanBackend.GetFrameObj();
	VulkanPresentation& presentationManager = _vulkanBackend.GetPresentationObj();


	_currentColorAttachment = presentationManager.GetSwapchainDesc().images[frameManager.GetCurrentImageIndex()].get();
	_currentDepthAttachment = _depthAttachments[frameManager.GetCurrentImageIndex()].get();


	ExecuteEntityCreateQueue();
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

	_viewDataBuffer->UploadData(0, &viewData, sizeof(ViewData)); // to verify this buffer
}

void SceneRenderer::UpdateDescriptors()
{
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
		_sceneDescriptorSets[descInd]->Write(0, _gBuffer.posIndex,    DescriptorType::COMBINED_IMAGE_SAMPLER,   _gBuffer.positions.get(), _samplerLinear.get());
		_sceneDescriptorSets[descInd]->Write(0, _gBuffer.normalIndex, DescriptorType::COMBINED_IMAGE_SAMPLER,   _gBuffer.normals.get(),   _samplerLinear.get());
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

	PipelineBarrierStorage pipelineBarriers;

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
	{
		Renderer::BeginRender({ _gBuffer.positions.get(), _gBuffer.normals.get(),
		_gBuffer.baseColor.get(), _gBuffer.metallicRoughness.get(), _currentDepthAttachment },
			glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

		// Opaque objects
		IndirectPushConst* opaqPushConst = new IndirectPushConst;
		opaqPushConst->vertexAddress = _meshDeviceBuffer.vertexBuffer->GetBufferAddress();
		opaqPushConst->commonMeshIndicesAddress = _indirectBuffer.commonIndicesBuffer->GetBufferAddress();
		opaqPushConst->commonMeshDataAddress = _indirectBuffer.commonDataBuffer->GetBufferAddress();
		opaqPushConst->viewDataAddress = _viewDataBuffer->GetBufferAddress();
		opaqPushConst->baseDrawOffset = 0;


		PushConsts opaqPushConstants;
		opaqPushConstants.data = (byte*)opaqPushConst;
		opaqPushConstants.size = sizeof(IndirectPushConst);

		RenderIndirectCountCommand opaqueCommand;
		opaqueCommand.buffer = _indirectBuffer.opaqueBuffer.get();
		opaqueCommand.indexBuffer = _meshDeviceBuffer.indexBuffer.get();
		opaqueCommand.descriptor = _sceneDescriptorSets[frameManager.GetCurrentFrameIndex()].get();
		opaqueCommand.pipeline = _gBufferPipeline.get();
		opaqueCommand.pushConstants = opaqPushConstants;
		opaqueCommand.maxDrawCount = _indirectBuffer.currentOpaqueSize; // count of different materials basically
		opaqueCommand.countBufferOffsetBytes = _indirectBuffer.countBufferOffset;

		Renderer::RenderIndirect(opaqueCommand);



		// Masked objects
		IndirectPushConst* maskedPushConst = new IndirectPushConst;
		maskedPushConst->vertexAddress = _meshDeviceBuffer.vertexBuffer->GetBufferAddress();
		maskedPushConst->commonMeshIndicesAddress = _indirectBuffer.commonIndicesBuffer->GetBufferAddress();
		maskedPushConst->commonMeshDataAddress = _indirectBuffer.commonDataBuffer->GetBufferAddress();
		maskedPushConst->viewDataAddress = _viewDataBuffer->GetBufferAddress();
		maskedPushConst->baseDrawOffset  = _indirectBuffer.currentOpaqueSize;

		PushConsts maskedPushConstants;
		maskedPushConstants.data = (byte*)maskedPushConst;
		maskedPushConstants.size = sizeof(IndirectPushConst);

		RenderIndirectCountCommand maskedCommand;
		maskedCommand.buffer = _indirectBuffer.maskBuffer.get();
		maskedCommand.descriptor = _sceneDescriptorSets[frameManager.GetCurrentFrameIndex()].get();
		maskedCommand.pipeline = _gBufferPipeline.get();
		maskedCommand.indexBuffer = _meshDeviceBuffer.indexBuffer.get();
		maskedCommand.maxDrawCount = _indirectBuffer.currentMaskedSize;
		maskedCommand.pushConstants = maskedPushConstants;////////////////////////
		maskedCommand.countBufferOffsetBytes = _indirectBuffer.countBufferOffset;

		Renderer::RenderIndirect(maskedCommand);

		Renderer::EndRender();
	}

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
	lightCullPushConst->lightsListAddress = _pointLightsBuffer->GetBufferAddress();
	lightCullPushConst->lightsCount = _pointLights.size();
	lightCullPushConst->lightsIndicesAddress = _lightCullStructures.lightIndicesBuffer->GetBufferAddress();
	lightCullPushConst->cameraDataAddress = _viewDataBuffer->GetBufferAddress();
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
	pbrPassPushConst->lightAddress = _pointLightsBuffer->GetBufferAddress();
	pbrPassPushConst->lightsIndicesAddress = _lightCullStructures.lightIndicesBuffer->GetBufferAddress();
	pbrPassPushConst->cameraDataAddress = _viewDataBuffer->GetBufferAddress();
	pbrPassPushConst->positionTextureIdx = _gBuffer.posIndex;
	pbrPassPushConst->normalsTextureIdx = _gBuffer.normalIndex;
	pbrPassPushConst->baseColorTextureIdx = _gBuffer.baseIndex;
	pbrPassPushConst->metallicRoughnessTextureIdx = _gBuffer.metallicRoughnessIndex;
	pbrPassPushConst->pointLightsCount = _pointLights.size();
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

}

void SceneRenderer::ExecuteEntityCreateQueue()
{
	while (!_entityCreateQueue.empty())
	{
		if (_entityCreateQueue.front() == nullptr)
		{
			_entityCreateQueue.pop();
			continue;
		}

		const Entity& entity = *_entityCreateQueue.front();

		MeshComponent* meshComp = entity.GetComponent<MeshComponent>();
		// Check if buffer exist
		if (meshComp != nullptr)
		{
			auto loadingResult = AssetManager::Get()->TryToLoadAndStoreMesh(meshComp->folderName, &_engineBase.GetImageManager());
			if (!loadingResult.has_value())
				return;

			u32 meshIndex = loadingResult->meshIndex;
			meshComp->meshIndex = meshIndex;
			meshComp->materialIndex = loadingResult->materialIndex;
			if (meshIndex != 0)
			{
				const std::vector<SubmeshDescription>* submeshes = AssetManager::Get()->GetAssetSubmeshes(meshIndex);
				if (submeshes == nullptr)
					return;

				for (auto submeshIt = submeshes->begin(); submeshIt != submeshes->end(); ++submeshIt)
				{

					const size_t vertexSize = submeshIt->vertexDesc.vertexCount * sizeof(Vertex);
					const size_t indexSize = submeshIt->vertexDesc.indexCount   * sizeof(u32);
					const size_t materialSize = submeshIt->materialDesc.materialsCount * sizeof(MaterialTexturesDesc);
					const size_t translationSize = sizeof(TransformComponent);

					// Add vertex buffer of this submesh to the end of the global mesh buffer
					_meshDeviceBuffer.vertexBuffer->UploadData(_meshDeviceBuffer.currentVertexOffset * sizeof(Vertex),
						submeshIt->vertexDesc.vertexPtr, vertexSize);

					_meshDeviceBuffer.indexBuffer->UploadData(_meshDeviceBuffer.currentIndexOffset   * sizeof(u32),
						submeshIt->vertexDesc.indicesPtr, indexSize);


					// Update common data buffer, it contains all the transformations, materials DATA
					CommonIndirectData commonData{};

					// Push all common mesh data into single buffer: material index, transformation index and their data
					const TransformComponent* transComp = entity.GetComponent<TransformComponent>();
					commonData.transformDesc = transComp ? *transComp : TransformComponent{};

					commonData.materialsDesc = submeshIt->materialDesc.materialTexturesPtr
						? *submeshIt->materialDesc.materialTexturesPtr : MaterialTexturesDesc{};


					_indirectBuffer.commonDataBuffer->UploadData(_indirectBuffer.currentCommonDataOffset * sizeof(CommonIndirectData),
						&commonData, sizeof(CommonIndirectData));

					_indirectBuffer.currentCommonDataOffset += 1;


					// Update common indices buffer, it contains all the transformation, materials INDICES
					CommonIndirectIndices commonIndices{};
					commonIndices.commonDataIndex = _indirectBuffer.currentCommonIndicesOffset;


					_indirectBuffer.commonIndicesBuffer->UploadData(_indirectBuffer.currentCommonIndicesOffset 
						* sizeof(CommonIndirectIndices),
						&commonIndices, sizeof(CommonIndirectIndices));


					_indirectBuffer.currentCommonIndicesOffset += 1;



					DrawIndexedIndirectCommand drawCommand;
					drawCommand.firstIndex = _meshDeviceBuffer.currentIndexOffset;
					drawCommand.firstInstance = 0;
					drawCommand.instanceCount = 1;
					drawCommand.indexCount = submeshIt->vertexDesc.indexCount;
					drawCommand.vertexOffset = _meshDeviceBuffer.currentVertexOffset;

					_indirectBuffer.opaqueBuffer->UploadData(_indirectBuffer.currentOpaqueSize * sizeof(DrawIndexedIndirectCommand),
						&drawCommand, sizeof(DrawCommand));
					_indirectBuffer.currentOpaqueSize += 1;

					// update count buffer
					_indirectBuffer.opaqueBuffer->UploadData(_indirectBuffer.countBufferOffset,
						&_indirectBuffer.currentOpaqueSize, sizeof(u32));
					//switch (submeshIt->alphaMode.type)
					//{
					//case AlphaMode::AlphaType::ALPHA_OPAQUE:
					//{
					//	_indirectBuffer.opaqueBuffer->UploadData(_indirectBuffer.currentOpaqueSize * sizeof(DrawIndexedIndirectCommand),
					//		&drawCommand, sizeof(DrawCommand));
					//	_indirectBuffer.currentOpaqueSize += 1;

					//	// update count buffer
					//	_indirectBuffer.opaqueBuffer->UploadData(_indirectBuffer.countBufferOffset,
					//		&_indirectBuffer.currentOpaqueSize, sizeof(u32));

					//	break;
					//}

					//case AlphaMode::AlphaType::ALPHA_MASK:
					//{
					//	_indirectBuffer.maskBuffer->UploadData(_indirectBuffer.currentMaskedSize * sizeof(DrawIndexedIndirectCommand),
					//		&drawCommand, sizeof(DrawCommand));

					//	_indirectBuffer.currentMaskedSize += 1;

					//	// update count buffer
					//	_indirectBuffer.maskBuffer->UploadData(_indirectBuffer.countBufferOffset,
					//		&_indirectBuffer.currentMaskedSize, sizeof(u32));
					//	break;
					//}

					//default:
					//	std::unreachable();
					//}


					_meshDeviceBuffer.currentVertexOffset += submeshIt->vertexDesc.vertexCount;
					_meshDeviceBuffer.currentIndexOffset  += submeshIt->vertexDesc.indexCount;
				}
			}
			else
			{
				std::cout << "Unable to create mesh buffers, asset manager returned index 0\n";
			}
		}

		// TO REPLACE!!!!!!!!!!!!
		_pointLightsBuffer->UploadData(0, _pointLights.data(), sizeof(PointLight)* _pointLights.size());

		_entityCreateQueue.pop();
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
void SceneRenderer::SubmitEntityToDraw(const Entity& entity)
{
	VulkanFrame& frameManager = _vulkanBackend.GetFrameObj();

	BufferManager& bufferManager = _engineBase.GetBufferManager();
	const ImageManager& imageManager = _engineBase.GetImageManager();


	_entityCreateQueue.push(&entity);

}