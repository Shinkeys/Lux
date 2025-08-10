#pragma once
#include "../base/core/renderer.h"
#include "../base/core/rendererAPI.h"
#include "../base/core/engine_base.h"
#include "../base/core/image.h"
#include "../base/core/descriptor.h"
#include "lights.h"
#include "iscene_renderer.h"
#include "../constructed_types/device_indexed_buffer.h"
#include "../constructed_types/device_indirect_buffer.h"

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

// Totally fine, would fit in 128 bytes easily.
struct LightCullPushConst
{
	VkDeviceAddress lightsListAddress{ 0 };
	VkDeviceAddress lightsIndicesAddress{ 0 };
	VkDeviceAddress cameraDataAddress{ 0 };

	u32 lightsCount{ 0 };
	u32 maxLightsPerCluster{ 0 };
	u32 tileSize{ 0 };
};

struct PBRPassPushConst
{
	VkDeviceAddress lightAddress{ 0 };
	VkDeviceAddress lightsIndicesAddress{ 0 };
	VkDeviceAddress cameraDataAddress{ 0 };
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
	std::unique_ptr<Buffer> lightIndicesBuffer{};

	glm::ivec3 numWorkGroups{ glm::ivec3(0) };
	u32 tilesPerScreen{ 0 }; // WOULD BE NUM WORK GROUPS.X * Y * Z

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

enum class MeshType : u8
{
	MESH_OPAQUE,
	MESH_MASK,
};

struct RenderInstance
{
	u32 materialIndex{ 0 };
	u32 meshIndex{ 0 };
	const TransformComponent* translation{ nullptr };
};

struct DrawIndexedIndirectCommand
{
	u32    indexCount{ 0 };
	u32    instanceCount{ 0 };
	u32    firstIndex{ 0 };
	i32    vertexOffset{ 0 };
	u32    firstInstance{ 0 };
};

struct CommonIndirectData
{
	MaterialTexturesDesc materialsDesc{};
	TransformComponent transformDesc{};

	float alphaCutoff{ 0.0f };
};


struct GBufferPipelines
{
	std::unique_ptr<Pipeline> opaquePipeline{ nullptr };
	std::unique_ptr<Pipeline> maskPipeline{ nullptr };
};

class Entity;
class SceneRenderer : public ISceneRenderer
{
private:
	EngineBase& _engineBase;

	std::unique_ptr<Pipeline> _pbrShadingPipeline;
	GBufferPipelines _gBufferPipelines;

	std::vector<std::unique_ptr<Descriptor>> _sceneDescriptorSets;

	LightCullingStructures _lightCullStructures;

	std::unique_ptr<Buffer> _baseMaterialsSSBO;
	std::unique_ptr<Sampler> _samplerLinear;
	std::unique_ptr<Sampler> _samplerNearest;


	std::vector<std::unique_ptr<Image>> _depthAttachments;

	Image* _currentDepthAttachment{nullptr};
	Image* _currentColorAttachment{nullptr};


	std::vector<PointLight> _pointLights;
	std::unique_ptr<Buffer> _pointLightsBuffer;

	GBuffer _gBuffer;

	std::unique_ptr<Buffer> _viewDataBuffer;

	void UpdateDescriptors();

	DeviceIndirectBuffer _indirectBuffer;
	DeviceIndexedBuffer  _meshDeviceBuffer;

	std::queue<const Entity*> _entityCreateQueue;

	void ExecuteEntityCreateQueue();
public:
	/**
	* @brief Pass the objects which would LIVE after the submission
	* @param entity reference
	*/
	void SubmitEntityToDraw(const Entity& entity);
	void Update(const Camera& camera) override;
	void Draw() override;

	SceneRenderer() = delete;
	SceneRenderer(EngineBase& engineBase);
	SceneRenderer(const SceneRenderer&) = delete;
	SceneRenderer(SceneRenderer&&) = delete;
	SceneRenderer& operator= (const SceneRenderer&) = delete;
	SceneRenderer& operator= (SceneRenderer&&) = delete;
};