#include "../../../headers/scene/RT_scene_renderer.h"
#include "../../../headers/base/core/engine_base.h"
#include "../../../headers/base/core/raytracing/RT_base.h"
#include "../../../headers/base/core/raytracing/RT_acceleration_structure.h"


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




	//std::unique_ptr<RTAccelerationStructure> tlasTest = engineBase.GetRayTracingManager().CreateTLAS(TLASSpecification{});

}

void RTSceneRenderer::Update(const Camera& camera)
{

}

void RTSceneRenderer::Draw()
{

}