#pragma once
#include "iscene_renderer.h"
#include "../constructed_types/device_indexed_buffer.h"
#include "../asset/asset_types.h"


struct RTPassPushConst
{
	u64 viewDataAddress{ 0 };
	u64 vertexAddress{ 0 };
	u64 indexAddress{ 0 };
	u64 meshesDataAddress{ 0 };
	u64 lightsAddress{ 0 };

	u32 maxRecursionDepth{ 4 };
};

struct RTMeshData
{
	MaterialTexturesDesc materialsDesc{};
	u32 vertexBufferOffset;
	u32 indexBufferOffset;
	float alphaCutoff{ 0.0f };
};


class Buffer;
struct RTMeshDataBuffer
{
	std::unique_ptr<Buffer> buffer{ nullptr };

	size_t offsetBytes{ 0 };
};

class EngineBase;
class Descriptor;
class Pipeline;
class Image;
class RTPipeline;
class RTAccelerationStructure;
class ShaderBindingTable;
class Sampler;
struct BLASContainer;
struct SubmeshDescription;

class RTSceneRenderer : public ISceneRenderer
{
private:
	EngineBase& _engineBase;

	std::vector<std::unique_ptr<Descriptor>> _sceneDescriptorSets;
	 
	std::unique_ptr<RTPipeline> _rtPipeline;

	std::unique_ptr<ShaderBindingTable> _sbt;

	std::vector<BLASContainer> _sceneBLASes;
	std::unique_ptr<RTAccelerationStructure> _sceneTLAS;

	std::unique_ptr<Buffer> _pointLightsBuffer;

	std::unique_ptr<Buffer> _viewDataBuffer;

	std::unique_ptr<Sampler> _samplerLinear;

	std::unique_ptr<Image> _outputTarget;

	RTMeshDataBuffer _meshesData;
	DeviceIndexedBuffer _meshDeviceBuffer; // All scene meshes in the buffer
	// TO REWORK THIS APPROACH!!!!!
	std::queue<const Entity*> _entityCreateQueue;

	void ExecuteEntityCreateQueue();
	void AddObjectToMeshSceneBuffer(const SubmeshDescription& desc);
	void UpdateMeshDataBuffer(const SubmeshDescription& desc, u32 vertexOffset, u32 indexOffset);
public:
	void Update(const Camera& camera) override;
	void Draw() override;
	void SubmitEntityToDraw(const Entity& entity) override;

	RTSceneRenderer() = delete;
	RTSceneRenderer(EngineBase& engineBase);
	RTSceneRenderer(const RTSceneRenderer&) = delete;
	RTSceneRenderer(RTSceneRenderer&&) = delete;
	RTSceneRenderer& operator= (const RTSceneRenderer&) = delete;
	RTSceneRenderer& operator= (RTSceneRenderer&&) = delete;

};