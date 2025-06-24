#include "../../headers/scene/scene_renderer.h"
#include "../../headers/scene/entity.h"

SceneRenderer::SceneRenderer(VulkanBase& vulkanBackend) : _vulkanBackend{vulkanBackend}
{
	constexpr i32 pushConstBufferCount = 2; // uniform + vertexBuff

	GraphicsPipeline baseShadingPipeline;
	baseShadingPipeline.shaderName = "shading";
	baseShadingPipeline.cullMode = VK_CULL_MODE_BACK_BIT;
	baseShadingPipeline.pushConstantSizeBytes = sizeof(VkDeviceAddress) * pushConstBufferCount;

	// other data is aight

	_baseShadingPair = _vulkanBackend.GetPipelineObj().CreatePipeline(baseShadingPipeline);
}


void SceneRenderer::UpdateBuffers(const Entity& entity)
{
	VulkanBuffer& bufferManager = _vulkanBackend.GetBufferObj();

	std::vector<Geometry> geometry
	{
		Geometry{glm::vec3(-1.0f, -1.0f,  1.0f)}, // front // 0
		Geometry{glm::vec3(-1.0f,  1.0f,  1.0f)}, // 1
		Geometry{glm::vec3(1.0f,   1.0f,  1.0f)}, // 2
		Geometry{glm::vec3(1.0f,  -1.0f,  1.0f)}, // 3
		 

		Geometry{glm::vec3(-1.0f, -1.0f, -1.0f)}, // back // 4
		Geometry{glm::vec3(-1.0f,  1.0f, -1.0f)}, // 5
		Geometry{glm::vec3(1.0f,   1.0f, -1.0f)}, // 6
		Geometry{glm::vec3(1.0f,  -1.0f, -1.0f)} // 7
	};
	// temp
	std::vector<u32> indices
	{
		1, 0, 2, 0, 3, 2,
		4, 0, 1, 4, 1, 5,
		6, 2, 3, 3, 7, 6,
		6, 4, 5, 7, 4, 6,
		2, 6, 1, 6, 5, 1,
		7, 3, 0, 4, 7, 0
	};

	// Check if buffer exist
	if (bufferManager.GetMeshBuffers(entity.GetID()) == nullptr)
	{
		MeshVertexBufferCreateDesc vertexDesc;
		vertexDesc.geometryPtr = geometry.data();
		vertexDesc.elementsCount = static_cast<u32>(geometry.size());
		vertexDesc.bufferUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		vertexDesc.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		vertexDesc.bufferMemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		MeshIndexBufferCreateDesc indexDesc;
		indexDesc.indicesPtr = indices.data();
		indexDesc.elementsCount = (u32)indices.size();
		indexDesc.bufferUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		indexDesc.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		indexDesc.bufferMemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		bufferManager.CreateMeshBuffers(vertexDesc, indexDesc, entity.GetID());
	}


	const bool isUniformCreated = bufferManager.GetUniformBuffer(entity.GetID()) != nullptr;
	if (isUniformCreated)
	{
		const CameraComponent* comp = entity.GetComponent<CameraComponent>();
		if (comp && comp->isActive)
		{
			UniformData uniform;
			uniform.view = comp->camera->GetViewMatrix();
			uniform.proj = comp->camera->GetProjectionMatrix();
			bufferManager.UpdateUniformBuffer(uniform, entity.GetID());
		}
	}
	else // create
	{
		UniformData uniform;
		const CameraComponent* comp = entity.GetComponent<CameraComponent>();
		if (comp && comp->isActive)
		{
			uniform.view = comp->camera->GetViewMatrix();
			uniform.proj = comp->camera->GetProjectionMatrix();
		}

		bufferManager.CreateUniformBuffer(uniform, entity.GetID());
	}
}

void SceneRenderer::SubmitEntityToDraw(const Entity& entity)
{
	VulkanBuffer& bufferManager = _vulkanBackend.GetBufferObj();
	VulkanFrame& frameManager = _vulkanBackend.GetFrameObj();

	// temp
	// temp
	std::vector<u32> indices
	{
		0, 1, 2, 0, 3, 2,
		0, 4, 1, 4, 1, 5,
		3, 2, 6, 3, 7, 6,
		5, 4, 6, 7, 4, 6,
		2, 6, 1, 6, 5, 1,
		3, 7, 0, 7, 4, 0
	};

	UpdateBuffers(entity);


	const MeshBuffers* meshBuffers = bufferManager.GetMeshBuffers(entity.GetID());
	assert(meshBuffers && "Mesh buffer for entity is empty");
	const UniformBuffer* uniformBuffer = bufferManager.GetUniformBuffer(entity.GetID());
	assert(meshBuffers && "Uniform buffer for entity is empty");


	VulkanDrawCommand command;
	command.pipeline = _baseShadingPair.pipeline;
	command.pipelineLayout = _baseShadingPair.pipelineLayout;
	command.objectBufferAddress = meshBuffers->GetDeviceAddress();
	command.uniformBufferAddress = uniformBuffer->GetDeviceAddress();
	command.indexCount = static_cast<u32>(indices.size());
	command.indexBuffer = bufferManager.GetMeshBuffers(entity.GetID())->GetVkIndexBuffer();
	frameManager.SubmitRenderTask(command); // Example. TO DO
}