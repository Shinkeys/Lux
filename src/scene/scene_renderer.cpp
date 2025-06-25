#include "../../headers/scene/scene_renderer.h"
#include "../../headers/scene/entity.h"

#include <glm/gtc/matrix_transform.hpp>

SceneRenderer::SceneRenderer(VulkanBase& vulkanBackend, AssetManager& manager) : _vulkanBackend{vulkanBackend}, _assetManager{manager}
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

	MeshComponent* meshComp = entity.GetComponent<MeshComponent>();
	// Check if buffer exist
	if (meshComp != nullptr && bufferManager.GetMeshBuffers(entity.GetID()) == nullptr)
	{
		u32 meshIndex = _assetManager.TryToLoadAndStoreMesh(meshComp->folderName);
		meshComp->meshIndex = meshIndex;
		if (meshIndex != 0)
		{
			const GeometryDescription* desc = _assetManager.GetGeometryDesc(meshIndex);
			if (desc != nullptr)
			{
				MeshVertexBufferCreateDesc vertexDesc;
				vertexDesc.geometryPtr = desc->geometryPtr;
				vertexDesc.elementsCount = desc->geometryCount;
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

	}


	const bool isUniformCreated = bufferManager.GetUniformBuffer(entity.GetID()) != nullptr;
	if (isUniformCreated)
	{
		const CameraComponent* comp = entity.GetComponent<CameraComponent>();
		const TranslationComponent* transComp = entity.GetComponent<TranslationComponent>();
		if (comp && comp->isActive)
		{
			glm::mat4 model = transComp ? GenerateModelMatrix(*transComp) : glm::mat4(1.0f);
			UniformData uniform{}	;
			uniform.view = comp->camera->GetViewMatrix();
			uniform.proj = comp->camera->GetProjectionMatrix();
			uniform.model = model;
			bufferManager.UpdateUniformBuffer(uniform, entity.GetID());
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

		bufferManager.CreateUniformBuffer(uniform, entity.GetID());
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

void SceneRenderer::SubmitEntityToDraw(const Entity& entity)
{
	VulkanBuffer& bufferManager = _vulkanBackend.GetBufferObj();
	VulkanFrame& frameManager = _vulkanBackend.GetFrameObj();

	UpdateBuffers(entity);


	const MeshBuffers* meshBuffers = bufferManager.GetMeshBuffers(entity.GetID());
	assert(meshBuffers && "Mesh buffer for entity is empty");
	const UniformBuffer* uniformBuffer = bufferManager.GetUniformBuffer(entity.GetID());
	assert(meshBuffers && "Uniform buffer for entity is empty");


	const MeshComponent* meshComp = entity.GetComponent<MeshComponent>();
	if (meshComp != nullptr)
	{
		const u32 meshAssetIndex = meshComp->meshIndex;
		if (meshAssetIndex == 0)
			return;

		const GeometryDescription* geometryDesc =  _assetManager.GetGeometryDesc(meshAssetIndex);
		if (geometryDesc == nullptr)
			return;

		VulkanDrawCommand command;
		command.pipeline = _baseShadingPair.pipeline;
		command.pipelineLayout = _baseShadingPair.pipelineLayout;
		command.objectBufferAddress = meshBuffers->GetDeviceAddress();
		command.uniformBufferAddress = uniformBuffer->GetDeviceAddress();
		command.indexCount = geometryDesc->indexCount;
		command.indexBuffer = bufferManager.GetMeshBuffers(entity.GetID())->GetVkIndexBuffer();
		frameManager.SubmitRenderTask(command); // Example. TO DO

	}

}