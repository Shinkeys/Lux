#pragma once
#include "iscene_renderer.h"

struct RTPassPushConst
{
	u64 viewDataAddress{ 0 };
};


class EngineBase;
class Descriptor;
class Buffer;
class Pipeline;
class Image;
class RTPipeline;
class RTAccelerationStructure;
class ShaderBindingTable;
struct BLASContainer;

class RTSceneRenderer : public ISceneRenderer
{
private:
	EngineBase& _engineBase;

	std::vector<std::unique_ptr<Descriptor>> _sceneDescriptorSets;
	 
	std::unique_ptr<RTPipeline> _rtPipeline;

	std::unique_ptr<ShaderBindingTable> _sbt;

	std::vector<BLASContainer> _sceneBLASes;
	std::unique_ptr<RTAccelerationStructure> _sceneTLAS;

	std::unique_ptr<Buffer> _viewDataBuffer;


	std::unique_ptr<Image> _outputTarget;

	// TO REWORK THIS APPROACH!!!!!
	std::queue<const Entity*> _entityCreateQueue;

	void ExecuteEntityCreateQueue();
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