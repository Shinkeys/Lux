#pragma once
#include "iscene_renderer.h"


class EngineBase;
class Descriptor;
class Buffer;
class RTPipeline;
class RTAccelerationStructure;
class ShaderBindingTable;

class RTSceneRenderer : public ISceneRenderer
{
private:
	EngineBase& _engineBase;

	std::vector<std::unique_ptr<Descriptor>> _sceneDescriptorSets;
	 
	std::unique_ptr<RTPipeline> _rtPipeline;

	std::unique_ptr<ShaderBindingTable> _sbt;

	std::unique_ptr<RTAccelerationStructure> _sceneBLAS;
	std::unique_ptr<RTAccelerationStructure> _sceneTLAS;

	std::unique_ptr<Buffer> _triangleBuffer;
	std::unique_ptr<Buffer> _triangleIndicesBuffer;
public:
	void Update(const Camera& camera) override;
	void Draw() override;

	RTSceneRenderer() = delete;
	RTSceneRenderer(EngineBase& engineBase);
	RTSceneRenderer(const RTSceneRenderer&) = delete;
	RTSceneRenderer(RTSceneRenderer&&) = delete;
	RTSceneRenderer& operator= (const RTSceneRenderer&) = delete;
	RTSceneRenderer& operator= (RTSceneRenderer&&) = delete;

};