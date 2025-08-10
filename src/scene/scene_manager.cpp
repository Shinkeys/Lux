#include "../../headers/scene/scene_manager.h"


SceneManager::SceneManager(EngineBase& engineBase, Window& window)
	: _engineBase{ engineBase }, _window { window }
{
	_storageInstance     = std::make_unique<SceneStorage>();
	_rendererInstance    = std::make_unique<SceneRenderer>(engineBase);
	_RTrendererInstance  = std::make_unique<RTSceneRenderer>(engineBase);
	_sceneInstance       = std::make_unique<SceneBase>(*_rendererInstance, *_storageInstance);
}


void SceneManager::Update()
{
	_sceneInstance->Update();
	_sceneInstance->UpdateWithKeys(_window);
	//_rendererInstance->Update(_sceneInstance->GetCamera());
	//_rendererInstance->Draw();
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//!!!!!!!!!!!!!!!!!TO MAKE INTERFACE BUTTON TO CHANGE BETWEEN THEM LATER!!!!!!!!!!!!!!!!!!!!!!!!!
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	_RTrendererInstance->Update(_sceneInstance->GetCamera());
	_RTrendererInstance->Draw();
}