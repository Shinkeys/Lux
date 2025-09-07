#include "../../../headers/scene/RT_scene_renderer.h"
#include "../../../headers/base/core/renderer.h"
#include "../../../headers/base/core/engine_base.h"
#include "../../../headers/base/core/image.h"
#include "../../../headers/base/core/frame_manager.h"
#include "../../../headers/base/core/pipeline.h"
#include "../../../headers/base/core/presentation_manager.h"
#include "../../../headers/base/core/descriptor.h"
#include "../../../headers/base/core/raytracing/RT_base.h"
#include "../../../headers/base/core/raytracing/RT_acceleration_structure.h"
#include "../../../headers/base/core/raytracing/RT_pipeline.h"
#include "../../../headers/base/core/raytracing/shader_binding_table.h"
#include "../../../headers/constructed_types/blas_container.h"
#include "../../../headers/util/camera_types.h"
#include "../../headers/scene/entity.h"


RTSceneRenderer::RTSceneRenderer(EngineBase& engineBase) : _engineBase{ engineBase }
{
	//std::vector<glm::vec3> triangleVertices
	//{
	//	{.5f,  -.5f, 0.0f},
	//	{0.0f,  .5f,  0.0f},
	//	{-.5f, -.5f, 0.0f},
	//};

	//std::vector<u32> triangleIndices
	//{
	//	0, 1, 2
	//};

	//BufferSpecification triangleBuffSpec{};
	//triangleBuffSpec.usage = BufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY |
	//	BufferUsage::SHADER_DEVICE_ADDRESS |
	//	BufferUsage::TRANSFER_DST;
	//triangleBuffSpec.size = triangleVertices.size() * sizeof(glm::vec3);
	//triangleBuffSpec.memoryProp = MemoryProperty::DEVICE_LOCAL;
	//triangleBuffSpec.memoryUsage = MemoryUsage::AUTO_PREFER_DEVICE;
	//triangleBuffSpec.allocCmdBuff = true;

	//std::unique_ptr<Buffer> triangleBuffer;
	//std::unique_ptr<Buffer> indexBuffer;

	//triangleBuffer = engineBase.GetBufferManager().CreateBuffer(triangleBuffSpec);
	//triangleBuffer->UploadData(0, triangleVertices.data(), triangleVertices.size() * sizeof(glm::vec3));

	//BufferSpecification triangleIndicesSpec{};
	//triangleIndicesSpec.usage = BufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY |
	//	BufferUsage::SHADER_DEVICE_ADDRESS |
	//	BufferUsage::TRANSFER_DST;
	//triangleIndicesSpec.size = triangleIndices.size() * sizeof(u32);
	//triangleIndicesSpec.memoryProp = MemoryProperty::DEVICE_LOCAL;
	//triangleIndicesSpec.memoryUsage = MemoryUsage::AUTO_PREFER_DEVICE;
	//triangleIndicesSpec.allocCmdBuff = true;

	//indexBuffer = engineBase.GetBufferManager().CreateBuffer(triangleIndicesSpec);
	//indexBuffer->UploadData(0, triangleIndices.data(), triangleIndices.size() * sizeof(u32));



	//BLASSpecification blasSpec{};
	//blasSpec.vertexAddress = triangleBuffer->GetBufferAddress();
	//blasSpec.indexAddress = indexBuffer->GetBufferAddress();
	//blasSpec.verticesCount = triangleVertices.size();
	//blasSpec.indicesCount = triangleIndices.size();
	//blasSpec.vertexStride = sizeof(glm::vec3);


	//BLASContainer blasContainer{};
	//blasContainer.accel = engineBase.GetRayTracingManager().CreateBLAS(blasSpec);

	//BLASInstance blasInstance{};
	//blasInstance.blasAddress = blasContainer.accel->GetAccelerationAddress();
	//blasInstance.customIndex = 0;
	//blasInstance.transform = glm::mat4(1.0f);
	//blasContainer.instance = blasInstance;

	//_sceneBLASes.push_back(std::move(blasContainer));

	// Descriptor
	// Create descriptor for every frame
	for (u32 i = 0; i < VulkanFrame::FramesInFlight; ++i)
	{
		constexpr i32 descriptorUsageCount = 3;
		DescriptorSpecification sceneDescSpec{};
		sceneDescSpec.bindings.resize(descriptorUsageCount);
		sceneDescSpec.bindings[0].binding = 0;
		sceneDescSpec.bindings[0].descriptorCount = 1;
		sceneDescSpec.bindings[0].descriptorType = DescriptorType::ACCELERATION_STRUCTURE;

		// Just an image where rt pipeline would store it's result
		sceneDescSpec.bindings[1].binding = 1;
		sceneDescSpec.bindings[1].descriptorCount = 1;
		sceneDescSpec.bindings[1].descriptorType = DescriptorType::STORAGE_IMAGE;

		// Just an image where rt pipeline would store it's result
		sceneDescSpec.bindings[2].binding = 2;
		sceneDescSpec.bindings[2].descriptorCount = 1024;
		sceneDescSpec.bindings[2].descriptorType = DescriptorType::COMBINED_IMAGE_SAMPLER;


		_sceneDescriptorSets.emplace_back(_engineBase.GetDescriptorManager().CreateDescriptorSet(sceneDescSpec));
	}

	// Sampler
	SamplerSpecification linearSpec;
	linearSpec.minFiltering = Filter::FILTER_LINEAR;
	linearSpec.magFiltering = Filter::FILTER_LINEAR;
	linearSpec.mipmapFiltering = SamplerMipMapMode::SAMPLER_MIPMAP_MODE_LINEAR;
	linearSpec.addressMode = SamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
	linearSpec.minLod = 0.0f;
	linearSpec.maxLod = 1000.0f;
	linearSpec.lodBias = 0.0f;


	_samplerLinear = _engineBase.GetImageManager().CreateSampler(linearSpec);

	auto extractRawPtrsLambda = [&]()
		{
			std::vector<Descriptor*> descriptors;

			for (auto& descriptor : _sceneDescriptorSets)
				descriptors.push_back(descriptor.get());

			return descriptors;
		};

	{
		// RT Pipeline
		RTPipelineSpecification pipelineSpec{};
		pipelineSpec.descriptorSets = extractRawPtrsLambda();
		pipelineSpec.pushConstantOffset = 0;
		pipelineSpec.pushConstantSizeBytes = sizeof(RTPassPushConst);

		std::vector<RTShaderSpec> shadersSpec{};

		RTShaderSpec raygenShader{};
		raygenShader.name = "raytracing";
		raygenShader.entryPoint = "RaygenMain";
		raygenShader.shaderGroup = RTShaderGroup::SHADER_STAGE_RAYGEN;

		RTShaderSpec missShader{};
		missShader.name = "raytracing";
		missShader.entryPoint = "MissMain";
		missShader.shaderGroup = RTShaderGroup::SHADER_STAGE_MISS;

		RTShaderSpec closestShader{};
		closestShader.name = "raytracing";
		closestShader.entryPoint = "ClosestMain";
		closestShader.shaderGroup = RTShaderGroup::SHADER_STAGE_CLOSEST_HIT;


		shadersSpec.push_back(raygenShader);
		shadersSpec.push_back(missShader);
		shadersSpec.push_back(closestShader);

		pipelineSpec.shaders = shadersSpec;

		_rtPipeline = _engineBase.GetPipelineManager().CreateRTPipeline(pipelineSpec);
	}

	RayTracingManager& rtManager = _engineBase.GetRayTracingManager();
	// SBT
	SBTSpecification sbtSpec{};
	sbtSpec.missCount = 1;
	sbtSpec.hitCount = 1;

	constexpr u32 handleCount = 3; // 1 raygen + 1 miss + 1 hit
	const u32 handleSize = rtManager.GetRTDeviceProperties().shaderGroupHandleSize;
	const u32 dataSize = handleCount * handleSize;
	sbtSpec.handles.resize(dataSize);
	rtManager.GetRTGroupHandles(_rtPipeline.get(), 0, handleCount, dataSize, sbtSpec.handles.data());
	_sbt = rtManager.CreateSBT(sbtSpec);

	const u32 windowWidth  = _engineBase.GetPresentationManager().GetSwapchainExtent().x;
	const u32 windowHeight = _engineBase.GetPresentationManager().GetSwapchainExtent().y;
	{
		ImageSpecification imageSpec;
		imageSpec.usage = ImageUsage::IMAGE_USAGE_STORAGE_BIT | ImageUsage::IMAGE_USAGE_TRANSFER_SRC;
		imageSpec.mipLevels = 1;
		imageSpec.aspect = ImageAspect::IMAGE_ASPECT_COLOR;
		imageSpec.extent = { windowWidth, windowHeight, 1 };
		imageSpec.format = ImageFormat::IMAGE_FORMAT_B8G8R8A8_UNORM;
		imageSpec.type = ImageType::IMAGE_TYPE_RENDER_TARGET;

		_outputTarget = _engineBase.GetImageManager().CreateImage(imageSpec);
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
}


void RTSceneRenderer::Update(const Camera& camera)
{
	ExecuteEntityCreateQueue();

	if (_sceneBLASes.empty())
		return;

	TLASSpecification tlasSpec{};
	
	for (u32 i = 0; i < _sceneBLASes.size(); ++i)
		tlasSpec.instances.push_back(_sceneBLASes[i].instance);

	_sceneTLAS = _engineBase.GetRayTracingManager().CreateTLAS(tlasSpec);

	//// Camera data buffer
	ViewData viewData;
	viewData.proj = camera.GetProjectionMatrix();
	viewData.view = camera.GetViewMatrix();
	viewData.inverseProjection = camera.GetInverseProjection();
	viewData.inverseView = camera.GetInverseView();
	viewData.viewProj = camera.GetViewProjectionMatrix();
	viewData.position = camera.GetPosition();
	viewData.nearPlane = camera.GetNearPlane();
	viewData.farPlane = camera.GetFarPlane();
	viewData.viewportExt = { _outputTarget->GetSpecification().extent.x, _outputTarget->GetSpecification().extent.y };

	_viewDataBuffer->UploadData(0, &viewData, sizeof(ViewData));



	for (u32 descInd = 0; descInd < VulkanFrame::FramesInFlight; ++descInd)
	{
		u32 availableIndex = 0;
		// Main shading pass
		const auto& allTextures = AssetManager::Get()->GetAllTextures(); // TEMPORARY SOLUTION. TO REWORK

		assert(!allTextures.empty() && "Images size is zero");
		for (u32 i = 0; i < allTextures.size(); ++i)
		{
			_sceneDescriptorSets[descInd]->Write(2, allTextures[i].first,
				DescriptorType::COMBINED_IMAGE_SAMPLER, allTextures[i].second.get(), _samplerLinear.get());
			++availableIndex;
		}
	}
}

void RTSceneRenderer::Draw()
{
	if (_sceneBLASes.empty())
		return;

	FrameManager& frameManager = _engineBase.GetFrameManager();

	const u32 currentFrameIdx = frameManager.GetCurrentFrameIndex();
	Image* currentColorAttachment = _engineBase.GetPresentationManager().GetSwapchainImage(frameManager.GetCurrentImageIndex());

	assert(currentColorAttachment && "Current color attachment is null");


	_sceneDescriptorSets[currentFrameIdx]->Write(0, 0, _sceneTLAS.get());
	_sceneDescriptorSets[currentFrameIdx]->Write(1, 0, nullptr, _outputTarget.get());

	// Opaque objects
	RTPassPushConst* rtPassPushConst = new RTPassPushConst;
	rtPassPushConst->viewDataAddress = _viewDataBuffer->GetBufferAddress();

	PushConsts rtPushConstants;
	rtPushConstants.data = (byte*)rtPassPushConst;
	rtPushConstants.size = sizeof(RTPassPushConst);


	RTDrawCommand rtDrawCommand{};
	rtDrawCommand.rtPipeline = _rtPipeline.get();
	rtDrawCommand.descriptor = _sceneDescriptorSets[frameManager.GetCurrentFrameIndex()].get();
	rtDrawCommand.sbt = _sbt.get();
	rtDrawCommand.pushConstants = rtPushConstants;

	Renderer::RenderRayTracing(rtDrawCommand);


	PipelineBarrierStorage pipelineBarriers;

	// Swapchain(target) image
	PipelineImageBarrierInfo preCopyLayoutTransition;
	preCopyLayoutTransition.srcStageMask = PipelineStage::RAY_TRACING_SHADER;
	preCopyLayoutTransition.dstStageMask = PipelineStage::ALL_TRANSFER;
	preCopyLayoutTransition.srcAccessMask = AccessFlag::SHADER_WRITE;
	preCopyLayoutTransition.dstAccessMask = AccessFlag::TRANSFER_READ;
	preCopyLayoutTransition.image = currentColorAttachment;
	preCopyLayoutTransition.newLayout = ImageLayout::IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	pipelineBarriers.imageBarriers.push_back(preCopyLayoutTransition);

	Renderer::ExecuteBarriers(pipelineBarriers);

	// Output from rt shader
	_outputTarget->SetLayout(ImageLayout::IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, AccessFlag::SHADER_WRITE, AccessFlag::TRANSFER_READ,
		PipelineStage::RAY_TRACING_SHADER, PipelineStage::ALL_TRANSFER);

	_outputTarget->CopyToImage(currentColorAttachment);

	// Swapchain(target) image
	PipelineImageBarrierInfo postCopyLayoutTransition;
	postCopyLayoutTransition.srcStageMask = PipelineStage::ALL_TRANSFER;
	postCopyLayoutTransition.dstStageMask = PipelineStage::COLOR_ATTACHMENT_OUTPUT;
	postCopyLayoutTransition.srcAccessMask = AccessFlag::TRANSFER_READ;
	postCopyLayoutTransition.dstAccessMask = AccessFlag::COLOR_ATTACHMENT_READ;
	postCopyLayoutTransition.image = currentColorAttachment;
	postCopyLayoutTransition.newLayout = ImageLayout::IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	pipelineBarriers.imageBarriers.push_back(postCopyLayoutTransition);

	Renderer::ExecuteBarriers(pipelineBarriers);

	_outputTarget->SetLayout(ImageLayout::IMAGE_LAYOUT_GENERAL, AccessFlag::TRANSFER_READ, AccessFlag::SHADER_WRITE,
		PipelineStage::ALL_TRANSFER, PipelineStage::RAY_TRACING_SHADER);

}

void RTSceneRenderer::ExecuteEntityCreateQueue()
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
		TransformComponent* transformComp = entity.GetComponent<TransformComponent>();
		// Check if buffer exist
		if (meshComp != nullptr)
		{
			auto loadingResult = AssetManager::Get()->TryToLoadAndStoreMesh(meshComp->folderName, &_engineBase.GetImageManager());
			if (!loadingResult.has_value())
			{
				_entityCreateQueue.pop();
				continue;
			}

			u32 meshIndex = loadingResult->meshIndex;
			meshComp->meshIndex = meshIndex;
			meshComp->materialIndex = loadingResult->materialIndex;

			const std::vector<SubmeshDescription>* submeshes = AssetManager::Get()->GetAssetSubmeshes(meshComp->meshIndex);
			if (submeshes == nullptr)
			{
				_entityCreateQueue.pop();
				continue;
			}

			for (auto submeshIt = submeshes->begin(); submeshIt != submeshes->end(); ++submeshIt)
			{
				BufferSpecification verticesBufferSpec{};
				verticesBufferSpec.usage = BufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY |
					BufferUsage::SHADER_DEVICE_ADDRESS |
					BufferUsage::TRANSFER_DST;
				verticesBufferSpec.size = submeshIt->vertexDesc.vertexCount * sizeof(Vertex);
				verticesBufferSpec.memoryProp = MemoryProperty::DEVICE_LOCAL;
				verticesBufferSpec.memoryUsage = MemoryUsage::AUTO_PREFER_DEVICE;
				verticesBufferSpec.allocCmdBuff = true;

				std::unique_ptr<Buffer> vertexBuffer;
				std::unique_ptr<Buffer> indexBuffer;


				vertexBuffer = _engineBase.GetBufferManager().CreateBuffer(verticesBufferSpec);
				vertexBuffer->UploadData(0, submeshIt->vertexDesc.vertexPtr, submeshIt->vertexDesc.vertexCount * sizeof(Vertex));

				BufferSpecification indicesBufferSpec{};
				indicesBufferSpec.usage = BufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY |
					BufferUsage::SHADER_DEVICE_ADDRESS |
					BufferUsage::TRANSFER_DST;
				indicesBufferSpec.size = submeshIt->vertexDesc.indexCount * sizeof(u32);
				indicesBufferSpec.memoryProp = MemoryProperty::DEVICE_LOCAL;
				indicesBufferSpec.memoryUsage = MemoryUsage::AUTO_PREFER_DEVICE;
				indicesBufferSpec.allocCmdBuff = true; // otherwise order of execution inside cmd buff will be broken

				indexBuffer = _engineBase.GetBufferManager().CreateBuffer(indicesBufferSpec);
				indexBuffer->UploadData(0, submeshIt->vertexDesc.indicesPtr, submeshIt->vertexDesc.indexCount * sizeof(u32));


				BLASContainer blasContainer{};

				BLASSpecification blasSpec{};
				blasSpec.vertexAddress = vertexBuffer->GetBufferAddress();
				blasSpec.indexAddress = indexBuffer->GetBufferAddress();
				blasSpec.verticesCount = submeshIt->vertexDesc.vertexCount;
				blasSpec.indicesCount = submeshIt->vertexDesc.indexCount;
				blasSpec.vertexStride = sizeof(Vertex);

				blasContainer.accel = _engineBase.GetRayTracingManager().CreateBLAS(blasSpec);

				BLASInstance blasInstance{};
				blasInstance.blasAddress = blasContainer.accel->GetAccelerationAddress();
				blasInstance.customIndex = meshComp->materialIndex;
				blasInstance.transform = transformComp ? transformComp->model : glm::mat4(1.0f);
				blasContainer.instance = blasInstance;

				_sceneBLASes.push_back(std::move(blasContainer));

			}

			_entityCreateQueue.pop();

		}
		else
			std::cout << "Can't create a BLAS for mesh, mesh component is null\n";
	}
}

void RTSceneRenderer::SubmitEntityToDraw(const Entity& entity)
{
	_entityCreateQueue.push(&entity);
}