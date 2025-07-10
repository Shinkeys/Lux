#pragma once
#include "../base/core/renderer.h"
#include "../base/core/rendererAPI.h"
#include "../base/core/engine_base.h"
#include "../base/core/image.h"
#include "../base/core/descriptor.h"
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
	std::unique_ptr<Image> positions;
	std::unique_ptr<Image> normals;
	std::unique_ptr<Image> baseColor;
	std::unique_ptr<Image> metallicRoughness;

	u32 posIndex{ 0 };
	u32 normalIndex{ 0 };
	u32 baseIndex{ 0 };
	u32 metallicRoughnessIndex{ 0 };
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
	VkDeviceAddress globalLightsCounterAddress{ 0 };

	u32 lightsCount{ 0 };
	u32 maxLightsPerCluster{ 0 };
	u32 tileSize{ 0 };
};

struct LightPassPushConst
{
	VkDeviceAddress lightAddress{ 0 };
	VkDeviceAddress lightsIndicesAddress{ 0 };
	u32 positionTextureIdx{ 0 };
	u32 normalsTextureIdx{ 0 };
	u32 baseColorTextureIdx{ 0 };
	u32 metallicRoughnessTextureIdx{ 0 };
	u32 pointLightsCount{ 0 };
	u32 tileSize{ 0 };
};


struct LightCullingStructures
{
	std::unique_ptr<Pipeline>  lightCullingPipeline{};

	std::unique_ptr<Image> lightsGrid;
	SSBOPair lightIndicesBuffer{};

	glm::ivec3 numWorkGroups{ glm::ivec3(0) };
	u32 tilesPerScreen{ 0 }; // WOULD BE NUM WORK GROUPS.X * Y * Z

	UBOPair globalLightsCountBuffer{};

	const u32 maxLightsPerCluster{ 64 };
	const u32 tileSize{ 16 };
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
	// MAKE ABSTRACTION TO REMOVE VULKAN BACKEND COMPLETELY
	VulkanBase& _vulkanBackend;
	EngineBase& _engineBase;

	std::unique_ptr<Pipeline> _baseShadingPipeline;
	std::unique_ptr<Pipeline> _gBufferPipeline;

	std::vector<std::unique_ptr<Descriptor>> _sceneDescriptorSets;

	LightCullingStructures _lightCullStructures;

	SSBOPair _baseMaterialsSSBO;
	std::unique_ptr<Sampler> _samplerLinear;


	std::vector<const Entity*> _drawCommands;

	// To rework image class
	std::vector<std::unique_ptr<Image>> _depthAttachments;

	Image* _currentDepthAttachment{nullptr};
	Image* _currentColorAttachment{nullptr};


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
	SceneRenderer(VulkanBase& vulkanBackend, EngineBase& engineBase);
	SceneRenderer(const SceneRenderer&) = delete;
	SceneRenderer(SceneRenderer&&) = delete;
	SceneRenderer& operator= (const SceneRenderer&) = delete;
	SceneRenderer& operator= (SceneRenderer&&) = delete;
};