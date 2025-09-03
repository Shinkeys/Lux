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


RTSceneRenderer::RTSceneRenderer(EngineBase& engineBase) : _engineBase{ engineBase }
{
	std::vector<glm::vec3> triangleVertices
	{
		{1.0f,  -1.0f, 0.0f},
		{0.0f,  1.0f,  0.0f},
		{-1.0f, -1.0f, 0.0f},

		{ 1.0f,  -1.0f, 0.0f },
		{0.0f,  1.0f,  0.0f},
		{-1.0f, -1.0f, 0.0f}
	};

	std::vector<u32> triangleIndices
	{
		0, 1, 2, 3, 4, 5
	};

	BufferSpecification triangleBuffSpec{};
	triangleBuffSpec.usage = BufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY | BufferUsage::SHADER_DEVICE_ADDRESS;
	triangleBuffSpec.size = triangleVertices.size() * sizeof(glm::vec3);
	triangleBuffSpec.memoryProp = MemoryProperty::DEVICE_LOCAL;
	triangleBuffSpec.memoryUsage = MemoryUsage::AUTO_PREFER_DEVICE;
	triangleBuffSpec.allocCmdBuff = true;

	_triangleBuffer = engineBase.GetBufferManager().CreateBuffer(triangleBuffSpec);
	_triangleBuffer->UploadData(0, triangleVertices.data(), triangleVertices.size() * sizeof(glm::vec3));

	BufferSpecification triangleIndicesSpec{};
	triangleIndicesSpec.usage =  BufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY | BufferUsage::SHADER_DEVICE_ADDRESS;
	triangleIndicesSpec.size = triangleIndices.size() * sizeof(u32);
	triangleIndicesSpec.memoryProp = MemoryProperty::DEVICE_LOCAL;
	triangleIndicesSpec.memoryUsage = MemoryUsage::AUTO_PREFER_DEVICE;
	triangleIndicesSpec.allocCmdBuff = true;

	_triangleIndicesBuffer = engineBase.GetBufferManager().CreateBuffer(triangleIndicesSpec);
	_triangleIndicesBuffer->UploadData(0, triangleIndices.data(), triangleIndices.size() * sizeof(u32));



	BLASSpecification blasSpec{};
	blasSpec.vertexAddress = _triangleBuffer->GetBufferAddress();
	blasSpec.indexAddress  = _triangleIndicesBuffer->GetBufferAddress();
	blasSpec.verticesCount = triangleVertices.size();
	blasSpec.indicesCount  = triangleIndices.size();
	blasSpec.vertexStride  = sizeof(glm::vec3);


	_sceneBLAS = engineBase.GetRayTracingManager().CreateBLAS(blasSpec);


	// Descriptor
	// Create descriptor for every frame
	for (u32 i = 0; i < VulkanFrame::FramesInFlight; ++i)
	{
		constexpr i32 descriptorUsageCount = 2;
		DescriptorSpecification sceneDescSpec{};
		sceneDescSpec.bindings.resize(descriptorUsageCount);
		sceneDescSpec.bindings[0].binding = 0;
		sceneDescSpec.bindings[0].descriptorCount = 1;
		sceneDescSpec.bindings[0].descriptorType = DescriptorType::ACCELERATION_STRUCTURE;

		// Just an image where rt pipeline would store it's result
		sceneDescSpec.bindings[1].binding = 1;
		sceneDescSpec.bindings[1].descriptorCount = 1;
		sceneDescSpec.bindings[1].descriptorType = DescriptorType::STORAGE_IMAGE;


		_sceneDescriptorSets.emplace_back(_engineBase.GetDescriptorManager().CreateDescriptorSet(sceneDescSpec));
	}

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
		pipelineSpec.pushConstantSizeBytes = 0;

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

	{
		PipelineSpecification spec{};
		spec.type = PipelineType::GRAPHICS_PIPELINE;
		spec.shaderName = "raytracing";
		spec.entryPoints = { "VertexMain", "FragmentMain" };
		spec.cullMode = CullMode::CULL_MODE_BACK;
		spec.pushConstantSizeBytes = 0;
		spec.descriptorSets = { extractRawPtrsLambda() };
		spec.depthCompare = CompareOP::COMPARE_OP_LESS;
		spec.depthWriteEnable = true;
		spec.depthTestEnable = true;
		spec.colorFormats = { ImageFormat::IMAGE_FORMAT_B8G8R8A8_SRGB };

		_rasterPipeline = _engineBase.GetPipelineManager().CreatePipeline(spec);
	}

	const u32 windowWidth  = _engineBase.GetPresentationManager().GetSwapchainExtent().x;
	const u32 windowHeight = _engineBase.GetPresentationManager().GetSwapchainExtent().y;
	{
		ImageSpecification imageSpec;
		imageSpec.usage = ImageUsage::IMAGE_USAGE_STORAGE_BIT;
		imageSpec.mipLevels = 1;
		imageSpec.aspect = ImageAspect::IMAGE_ASPECT_COLOR;
		imageSpec.extent = { windowWidth, windowHeight, 1 };
		imageSpec.format = ImageFormat::IMAGE_FORMAT_R16G16B16A16_SFLOAT;
		imageSpec.type = ImageType::IMAGE_TYPE_RENDER_TARGET;

		_outputTarget = _engineBase.GetImageManager().CreateImage(imageSpec);
	}
}

void RTSceneRenderer::Update(const Camera& camera)
{
	TLASSpecification tlasSpec{};

	BLASInstances blasInstance{};
	blasInstance.blasAddress = _sceneBLAS->GetAccelerationAddress();
	blasInstance.transform = glm::mat4(1.0f);
	blasInstance.customIndex = 0;

	tlasSpec.instances = { blasInstance };

	_sceneTLAS = _engineBase.GetRayTracingManager().CreateTLAS(tlasSpec);

}

void RTSceneRenderer::Draw()
{
	FrameManager& frameManager = _engineBase.GetFrameManager();

	const u32 currentFrameIdx = frameManager.GetCurrentFrameIndex();
	Image* currentColorAttachment = _engineBase.GetPresentationManager().GetSwapchainImage(frameManager.GetCurrentImageIndex());

	assert(currentColorAttachment && "Current color attachment is null");


	_sceneDescriptorSets[currentFrameIdx]->Write(0, 0, _sceneTLAS.get());
	_sceneDescriptorSets[currentFrameIdx]->Write(1, 0, nullptr, _outputTarget.get());



	RTDrawCommand rtDrawCommand{};
	rtDrawCommand.rtPipeline = _rtPipeline.get();
	rtDrawCommand.descriptor = _sceneDescriptorSets[frameManager.GetCurrentFrameIndex()].get();
	rtDrawCommand.sbt = _sbt.get();
	 //rtDrawCommand.pushC
	Renderer::RenderRayTracing(rtDrawCommand);

	PipelineBarrierStorage pipelineBarriers;

	PipelineMemoryBarrierInfo preRasterizationBarrier;
	preRasterizationBarrier.srcStageMask = PipelineStage::RAY_TRACING_SHADER;
	preRasterizationBarrier.dstStageMask = PipelineStage::FRAGMENT_SHADER;
	preRasterizationBarrier.srcAccessMask = AccessFlag::STORAGE_WRITE;
	preRasterizationBarrier.dstAccessMask = AccessFlag::STORAGE_READ;
	pipelineBarriers.memoryBarriers.push_back(preRasterizationBarrier);


	Renderer::ExecuteBarriers(pipelineBarriers);

	// LIGHT PASS
	DrawCommand quadDrawCommand{};
	quadDrawCommand.pipeline = _rasterPipeline.get();
	quadDrawCommand.descriptor = _sceneDescriptorSets[frameManager.GetCurrentFrameIndex()].get();

	Renderer::BeginRender({ currentColorAttachment }, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	Renderer::RenderQuad(quadDrawCommand);
	Renderer::EndRender();
	
}