#pragma once
#include "../base/gfx/vk_base.h"
#include "../asset/asset_manager.h"
#include "camera.h"


class Entity;
class SceneStorage;
class SceneRenderer;
class RTSceneRenderer;
// Purpose: class to handle basic scene logic:
class SceneBase
{
private:
	SceneRenderer& _rendererInstance;	  // to rework this approach
	RTSceneRenderer& _rtRendererInstance; // to rework this approach
	SceneStorage& _storageInstance;
	//std::unordered_map<i32, Mesh> _entityMeshPtrs;

	void Initialize();

	std::shared_ptr<Camera> _camera; // basic camera object from which every component would copy;
public:
	/**
	* @brief return a copy. Perform operations on the copies and then upload them to the registry 
	*/
	//Entity CreateEntityInRegistry();
	//const auto& GetRegistry() const { return _entityRegistry; }
	ComponentList* GetComponentListByEntity(const Entity& entity);

	const Camera& GetCamera() const { assert(_camera && "Camera is nullptr somehow"); return *_camera; }
	void Update();
	void UpdateWithKeys(const Window& window);

	SceneBase() = delete;
	SceneBase(SceneRenderer& renderer, RTSceneRenderer& rtRenderer, SceneStorage& storage);

	SceneBase* s_Instance;
};