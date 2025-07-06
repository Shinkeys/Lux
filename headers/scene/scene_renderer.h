#pragma once
#include "../base/core/renderer.h"
#include "../base/core/rendererAPI.h"
#include "lights.h"

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

struct GBuffer
{
	// To rework image class
	std::shared_ptr<ImageHandle> positions;
	std::shared_ptr<ImageHandle> normals;
	std::shared_ptr<ImageHandle> baseColor;
	std::shared_ptr<ImageHandle> metallicRoughness;
};

struct GBufferPushConst
{
	VkDeviceAddress vertexAddress{ 0 };
	VkDeviceAddress uniformAddress{ 0 };
	VkDeviceAddress materialAddress{ 0 };
};


struct LightPassPushConst
{
	VkDeviceAddress lightAddress{ 0 };
	u32 positionTextureIdx{ 0 };
	u32 normalsTextureIdx{ 0 };
	u32 baseColorTextureIdx{ 0 };
	u32 metallicRoughnessTextureIdx{ 0 };
	u32 pointLightsCount{ 0 };
};

class Entity;
class SceneRenderer
{
private:
	VulkanBase& _vulkanBackend;
	PipelinePair _baseShadingPair;
	std::vector<DescriptorSet> _gBuffDescriptorSets;
	std::vector<DescriptorSet> _baseShadingDescriptorSets;
	SSBOPair _baseMaterialsSSBO;
	VkSampler _samplerLinear{ VK_NULL_HANDLE };

	using EntityID = u32;
	glm::mat4 GenerateModelMatrix(const TranslationComponent& translationComp);

	std::vector<const Entity*> _drawCommands;

	// To rework image class
	std::vector<std::shared_ptr<ImageHandle>> _depthAttachments;

	std::shared_ptr<ImageHandle> _currentDepthAttachment;
	std::shared_ptr<ImageHandle> _currentColorAttachment;


	std::vector<PointLight> _pointLights;
	SSBOPair _pointLightsBuffer;

	GBuffer _gBuffer;
	PipelinePair _gBufferPipeline;

public:
	/**
	* @brief Pass the objects which would LIVE after the submission
	* @param entity reference
	*/
	void SubmitEntityToDraw(const Entity& entity);
	void UpdateBuffers(const Entity& entity);
	//template<typename T>
	//void SubmitDataToBind();
	void Update();
	void Draw();

	SceneRenderer() = delete;
	~SceneRenderer() = default;
	SceneRenderer(VulkanBase& vulkanBackend);
	SceneRenderer(const SceneRenderer&) = delete;
	SceneRenderer(SceneRenderer&&) = delete;
	SceneRenderer& operator= (const SceneRenderer&) = delete;
	SceneRenderer& operator= (SceneRenderer&&) = delete;
};