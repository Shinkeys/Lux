#include "../../../headers/scene/RT_scene_renderer.h"
#include "../../../headers/base/core/engine_base.h"
#include "../../../headers/base/core/descriptor.h"
#include "../../../headers/base/core/raytracing/RT_base.h"
#include "../../../headers/base/core/raytracing/RT_acceleration_structure.h"
#include "../../../headers/base/core/raytracing/RT_pipeline.h"


RTSceneRenderer::RTSceneRenderer(EngineBase& engineBase) : _engineBase{engineBase}
{
	std::vector<glm::vec3> triangleVertices
	{
		{1.0f,  -1.0f, 0.0f},
		{0.0f,  1.0f,  0.0f},
		{-1.0f, -1.0f, 0.0f}
	};

	std::vector<u32> triangleIndices
	{
		0, 1, 2
	};

	BufferSpecification triangleBuffSpec{};
	triangleBuffSpec.usage = BufferUsage::VERTEX_BUFFER | BufferUsage::SHADER_DEVICE_ADDRESS;
	triangleBuffSpec.size = triangleVertices.size() * sizeof(glm::vec3);
	triangleBuffSpec.memoryProp = MemoryProperty::DEVICE_LOCAL;
	triangleBuffSpec.memoryUsage = MemoryUsage::AUTO_PREFER_DEVICE;

	std::unique_ptr<Buffer> triangleBuffer = engineBase.GetBufferManager().CreateBuffer(triangleBuffSpec);

	BufferSpecification triangleIndicesSpec{};
	triangleIndicesSpec.usage = BufferUsage::INDEX_BUFFER | BufferUsage::SHADER_DEVICE_ADDRESS;
	triangleIndicesSpec.size = triangleIndices.size() * sizeof(u32);
	triangleIndicesSpec.memoryProp = MemoryProperty::DEVICE_LOCAL;
	triangleIndicesSpec.memoryUsage = MemoryUsage::AUTO_PREFER_DEVICE;

	std::unique_ptr<Buffer> triangleIndicesBuffer = engineBase.GetBufferManager().CreateBuffer(triangleIndicesSpec);


	BLASSpecification blasSpec{};
	blasSpec.vertexAddress = triangleBuffer->GetBufferAddress();
	blasSpec.indexAddress  = triangleIndicesBuffer->GetBufferAddress();
	blasSpec.verticesCount = triangleVertices.size();
	blasSpec.indicesCount  = triangleIndices.size();
	blasSpec.vertexStride = sizeof(glm::vec3);

	std::unique_ptr<RTAccelerationStructure> blasTest = engineBase.GetRayTracingManager().CreateBLAS(blasSpec);

	TLASSpecification tlasSpec{};
	
	BLASInstances blasInstance{};
	blasInstance.blasAddress = blasTest->GetAccelerationAddress();
	blasInstance.transform = glm::mat4(1.0f);
	blasInstance.customIndex = 0;


	tlasSpec.instances = { blasInstance };

	std::unique_ptr<RTAccelerationStructure> tlasTest = engineBase.GetRayTracingManager().CreateTLAS(tlasSpec);
	

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


}

void RTSceneRenderer::Update(const Camera& camera)
{

}

void RTSceneRenderer::Draw()
{

}