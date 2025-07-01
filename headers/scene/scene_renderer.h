#pragma once
#include "../base/core/renderer.h"

// TO replace
struct UniformData
{
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view  = glm::mat4(1.0f);
	glm::mat4 proj  = glm::mat4(1.0f);
};

struct RenderData
{
	const VertexDescription* meshDesc{ nullptr };
	const MaterialDescription* materialDesc{ nullptr };
};

class Entity;
class SceneRenderer
{
private:
	VulkanBase& _vulkanBackend;
	PipelinePair _baseShadingPair;
	std::vector<DescriptorSet> _baseShadingDescriptorSets;
	SSBOPair _baseMaterialsSSBO;
	VkSampler _samplerLinear{ VK_NULL_HANDLE };

	using EntityID = u32;
	glm::mat4 GenerateModelMatrix(const TranslationComponent& translationComp);

	std::queue<VulkanDrawCommand> _drawCommands;
public:
	/**
	* @brief Pass the objects which would LIVE after the submission
	* @param entity reference
	*/
	void SubmitEntityToDraw(const Entity& entity);
	void UpdateBuffers(const Entity& entity);
	//template<typename T>
	//void SubmitDataToBind();
	void Update() const;
	void Draw();

	SceneRenderer() = delete;
	~SceneRenderer() = default;
	SceneRenderer(VulkanBase& vulkanBackend);
	SceneRenderer(const SceneRenderer&) = delete;
	SceneRenderer(SceneRenderer&&) = delete;
	SceneRenderer& operator= (const SceneRenderer&) = delete;
	SceneRenderer& operator= (SceneRenderer&&) = delete;
};