#pragma once
#include "../util/util.h"
#include "scene_base.h"


// Operate on copies and store them to the storage!
class Entity
{
private:
	SceneBase* _scene{ nullptr };
	i32 _id{-1};

	// Mesh* _mesh;
public:
	Entity() = delete;
	Entity(i32 id, SceneBase* scene) : _id{ id }, _scene{ scene }
	{

	}

	bool operator==(const Entity& other) const
	{
		return _id == other._id;
	}


	i32 GetID() const { return _id; }
	const SceneBase* GetScenePtr() const { return _scene; }


	template<typename Component>
	const Component* GetComponent() const 
	{
		assert(_scene&& _id != -1 && "Cannot get component for the object, scenePtr or id is null");
		auto entityComponentList = _scene->GetComponentListByEntity(*this);
		if (entityComponentList == nullptr)
			assert(entityComponentList && "Component list is empty in GetComponent(). Cannot add component. Check call stack");

		return entityComponentList->Get<Component>();
	}

	template<typename Component, typename... Args>
	Component& AddComponent(Args&&... args) const
	{
		assert(_scene && _id != -1 && "Cannot add component to the object, scenePtr or id is null");
		auto entityComponentList = _scene->GetComponentListByEntity(*this);
		if (entityComponentList == nullptr)
			assert(entityComponentList && "Component list is empty in GetComponent(). Cannot add component. Check call stack");

		return entityComponentList->Emplace<Component>(std::forward<Args>(args)...);
	}

};

// Maybe in the future engine would need multiple scenes.
template <>
struct std::hash<Entity>
{
	std::size_t operator()(const Entity& e) const noexcept
	{
		std::size_t h1 = std::hash<i32>{}(e.GetID());
		std::size_t h2 = std::hash<const SceneBase*>{}(e.GetScenePtr());
		return h1 ^ (h2 << 1);
	}
};