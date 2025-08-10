#pragma once
#include "scene_base.h"
#include "scene_renderer.h"
#include "RT_scene_renderer.h"
#include "scene_storage.h"
#include "../base/gfx/vk_base.h"
#include "../base/core/engine_base.h"



class SceneManager
{
private:
	Window& _window;
	EngineBase& _engineBase;

	std::unique_ptr<SceneRenderer> _rendererInstance;
	std::unique_ptr<RTSceneRenderer> _RTrendererInstance;
	std::unique_ptr<SceneBase> _sceneInstance;
	std::unique_ptr<SceneStorage> _storageInstance;

public:
	void Update();

	SceneManager() = delete;
	SceneManager(EngineBase& engineBase, Window& window);



};