#pragma once
#include "../scene/scene_base.h"
#include "../scene/scene_renderer.h"
#include "../scene/scene_storage.h"
#include "../base/gfx/vk_base.h"



class SceneManager
{
private:
	VulkanBase& _vulkanBackend;
	Window& _window;
	AssetManager _assetManager;

	std::unique_ptr<SceneRenderer> _rendererInstance;
	std::unique_ptr<SceneBase> _sceneInstance;
	std::unique_ptr<SceneStorage> _storageInstance;

public:
	void Update();

	SceneManager() = delete;
	SceneManager(VulkanBase& vulkanBackend, Window& window);



};