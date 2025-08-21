#pragma once
#include "iscene_renderer.h"

class EngineBase;
class Descriptor;
class RTPipeline;

class RTSceneRenderer : public ISceneRenderer
{
private:
	EngineBase& _engineBase;

	std::vector<std::unique_ptr<Descriptor>> _sceneDescriptorSets;

	std::unique_ptr<RTPipeline> _rtPipeline;

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