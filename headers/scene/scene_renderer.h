#pragma once
#include "../base/core/renderer.h"

// TO replace
struct UniformData
{
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view  = glm::mat4(1.0f);
	glm::mat4 proj  = glm::mat4(1.0f);
};

class Entity;
class SceneRenderer
{
private:
	VulkanBase& _vulkanBackend;
	AssetManager& _assetManager;
	PipelinePair _baseShadingPair;

	using EntityID = u32;
	glm::mat4 GenerateModelMatrix(const TranslationComponent& translationComp);
public:
	/**
	* @brief Pass the objects which would LIVE after the submission
	* @param entity reference
	*/
	void SubmitEntityToDraw(const Entity& entity);
	void UpdateBuffers(const Entity& entity);
	//template<typename T>
	//void SubmitDataToBind();


	SceneRenderer() = delete;
	~SceneRenderer() = default;
	SceneRenderer(VulkanBase& vulkanBackend, AssetManager& manager);
	SceneRenderer(const SceneRenderer&) = delete;
	SceneRenderer(SceneRenderer&&) = delete;
	SceneRenderer& operator= (const SceneRenderer&) = delete;
	SceneRenderer& operator= (SceneRenderer&&) = delete;
};