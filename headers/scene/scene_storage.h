#pragma once
#include "component.h"
#include "entity.h"

class SceneStorage
{	
private:
	u32 _currentAvailableID{ 1 };
	std::unordered_map<Entity, ComponentList> _entityRegistry;
public:
	const Entity& CreateEntityInRegistry(SceneBase* scene);
	const auto& GetRegistry() const { return _entityRegistry; }
	ComponentList* GetComponentListByEntity(const Entity& entity);
};