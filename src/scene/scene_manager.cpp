#include "../../headers/scene/scene_manager.h"


SceneManager::SceneManager(VulkanBase& vulkanBackend, Window& window) 
						   : _vulkanBackend{vulkanBackend}, _window{window}
{
	_storageInstance = std::make_unique<SceneStorage>();
	_rendererInstance = std::make_unique<SceneRenderer>(vulkanBackend, _assetManager);
	_sceneInstance = std::make_unique<SceneBase>(*_rendererInstance, *_storageInstance);
}


void SceneManager::Update()
{
	_sceneInstance->Update();
	_sceneInstance->UpdateWithKeys(_window);
}