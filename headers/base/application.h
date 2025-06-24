#pragma once
#include "core.h"
#include "window.h"
#include "gfx/vk_base.h"
#include "../util/util.h"
#include "../base/core/renderer.h"
#include "../scene/scene_manager.h"

#include <iostream>

class Application
{
private:
	Core _core;
	Window _window;
	VulkanBase _vulkanBackend;
	std::unique_ptr<SceneManager> _sceneManager;
	void Render();
	void Update();
	void Cleanup();
public:
	void Run();
};