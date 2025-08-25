	#pragma once
#include "core.h"
#include "window.h"
#include "gfx/vk_base.h"
#include "core/engine_base.h"
#include "../util/util.h"
#include "../base/core/renderer.h"
#include "../scene/scene_manager.h"
#include "../asset/asset_manager.h"

#include <iostream>

class Application
{
private:
	VulkanBase _vulkanBackend;
	Core _core;
	Window _window;
	std::unique_ptr<EngineBase> _engineBase;
	std::unique_ptr<SceneManager> _sceneManager;
	void Render();
	void Update();
	void Cleanup();
public:
	void Run();
};