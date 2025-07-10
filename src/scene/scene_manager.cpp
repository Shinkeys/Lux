#include "../../headers/scene/scene_manager.h"


SceneManager::SceneManager(VulkanBase& vulkanBackend, EngineBase& engineBase, Window& window)
	: _vulkanBackend{ vulkanBackend }, _engineBase{ engineBase }, _window { window }
{
	_storageInstance = std::make_unique<SceneStorage>();
	_rendererInstance = std::make_unique<SceneRenderer>(vulkanBackend, engineBase);
	_sceneInstance = std::make_unique<SceneBase>(*_rendererInstance, *_storageInstance);
}


void SceneManager::Update()
{
	_sceneInstance->Update();
	_sceneInstance->UpdateWithKeys(_window);
	_rendererInstance->Update(_sceneInstance->GetCamera());
	_rendererInstance->Draw();
}