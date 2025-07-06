#pragma once
#include "../base/core/renderer.h"
#include "../base/core/rendererAPI.h"
#include "lights.h"

// TO replace
struct EntityUniformData
{
	glm::mat4 model = glm::mat4(1.0f);
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
	VkDeviceAddress entityUniformAddress{ 0 };
	VkDeviceAddress materialAddress{ 0 };
	VkDeviceAddress cameraDataAddress{ 0 };
};

// Totally fine, would fit in 128 bytes easily.
struct LightCullPushConst
{
	VkDeviceAddress lightsListAddress{ 0 };
	VkDeviceAddress lightsIndicesAddress{ 0 };
	VkDeviceAddress cameraDataAddress{ 0 };

	u32 maxLightsPerCluster{ 0 };
	u32 lightsCount{ 0 };
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


struct LightCullingStructures
{
	std::vector<DescriptorSet> lightCullingDescriptorSets{};
	PipelinePair  lightCullingPipeline{};

	std::shared_ptr<ImageHandle> lightsGrid;
	SSBOPair lightIndicesBuffer{};

	glm::ivec3 numWorkGroups{ glm::ivec3(0) };
	u32 tilesPerScreen{ 0 }; // WOULD BE NUM WORK GROUPS.X * Y * Z

	const u32 maxLightsPerCluster{ 64 };
};

struct ViewData
{
	glm::mat4 view{ glm::mat4(1.0f) };
	glm::mat4 proj{ glm::mat4(1.0f) };
	glm::mat4 viewProj{ glm::mat4(1.0f) };
	glm::mat4 inverseProjection{ glm::mat4(1.0f) };
	glm::vec3 position{ glm::vec3(0.0f) };


	glm::ivec2 viewportExt{ glm::ivec2(0) }; 
	float nearPlane{ 0.0f };
	float farPlane{ 0.0f };
};


class Entity;
class SceneRenderer
{
private:
	VulkanBase& _vulkanBackend;
	PipelinePair _baseShadingPipeline;
	PipelinePair _gBufferPipeline;

	std::vector<DescriptorSet> _gBuffDescriptorSets;
	std::vector<DescriptorSet> _baseShadingDescriptorSets;

	LightCullingStructures _lightCullStructures;

	SSBOPair _baseMaterialsSSBO;
	VkSampler _samplerLinear{ VK_NULL_HANDLE };


	std::vector<const Entity*> _drawCommands;

	// To rework image class
	std::vector<std::shared_ptr<ImageHandle>> _depthAttachments;

	std::shared_ptr<ImageHandle> _currentDepthAttachment;
	std::shared_ptr<ImageHandle> _currentColorAttachment;


	std::vector<PointLight> _pointLights;
	SSBOPair _pointLightsBuffer;

	GBuffer _gBuffer;

	UBOPair _viewDataBuffer;
	UBOPair _baseTransformationBuffer;

	glm::mat4 GenerateModelMatrix(const TranslationComponent& translationComp);
	void UpdateDescriptors();
public:
	/**
	* @brief Pass the objects which would LIVE after the submission
	* @param entity reference
	*/
	void SubmitEntityToDraw(const Entity& entity);
	void UpdateBuffers(const Entity& entity);
	//template<typename T>
	//void SubmitDataToBind();
	void Update(const Camera& camera);
	void Draw();

	SceneRenderer() = delete;
	~SceneRenderer() = default;
	SceneRenderer(VulkanBase& vulkanBackend);
	SceneRenderer(const SceneRenderer&) = delete;
	SceneRenderer(SceneRenderer&&) = delete;
	SceneRenderer& operator= (const SceneRenderer&) = delete;
	SceneRenderer& operator= (SceneRenderer&&) = delete;
};