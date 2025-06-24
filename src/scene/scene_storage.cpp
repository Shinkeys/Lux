#include "../../headers/scene/scene_storage.h"
#include "../../headers/scene/entity.h"

const Entity& SceneStorage::CreateEntityInRegistry(SceneBase* scene)
{
	auto it = _entityRegistry.emplace(Entity{ _currentAvailableID++, scene }, ComponentList{});

	// TO DO
	// _entityMeshPtrs.emplace(_currentAvailableID, AssetManager::GetData());


	return it.first->first;
}

ComponentList* SceneStorage::GetComponentListByEntity(const Entity& entity)
{
	auto it = _entityRegistry.find(entity);

	if (it == _entityRegistry.end())
		return nullptr;

	return &it->second;
}