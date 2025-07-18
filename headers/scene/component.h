#pragma once
#include "../util/util.h"
#include "../util/logger.h"
#include "camera.h"

#include <glm/glm.hpp>

struct TagComponent
{
	std::string _tag{ "" };

	TagComponent() = default;
	TagComponent(const std::string& tag) : _tag{ tag } { }


	const std::string GetTag() { return _tag; }
};


struct TranslationComponent
{
	//glm::mat4 rotation{ glm::mat4(1.0f) }; // To do: quats
	//glm::vec3 translation{ glm::vec3(0.0f) };
	//glm::vec3 scale{ glm::vec3(0.0f) };

	glm::mat4 model{ glm::mat4(1.0f) };

	TranslationComponent() = default;
	TranslationComponent(const glm::mat4& newModel) : model{newModel}{}
	/*TranslationComponent(const glm::mat4& newRot, glm::vec3 newTranslation, glm::vec3 newScale) :
		rotation{newRot}, translation{newTranslation}, scale{newScale}
	{
	}*/
};

struct CameraComponent
{
	std::shared_ptr<Camera> camera{ nullptr };
	bool isActive{ false };

	CameraComponent() = default;
	CameraComponent(std::shared_ptr<Camera> newCamera, bool status) : camera{ newCamera }, isActive{status}
	{
	}
};

struct MeshComponent
{
	std::string folderName = "";

	u32 meshIndex{ 0 };
	u32 materialIndex{ 0 };

};

class ComponentList
{
private:
	std::optional<TagComponent> _tagComponent;
	std::optional<TranslationComponent> _translationComponent;
	std::optional<CameraComponent> _cameraComponent;
	std::optional<MeshComponent> _meshComponent;
public:
	template<typename Component, typename... Args>
	Component& Emplace(Args&&... args)
	{
		if constexpr (std::is_same<Component, TagComponent>::value)
		{
			return _tagComponent.emplace(args...);
		}
		else if constexpr (std::is_same<Component, TranslationComponent>::value)
		{
			return _translationComponent.emplace(args...);
		}
		else if constexpr (std::is_same<Component, CameraComponent>::value)
		{
			return _cameraComponent.emplace(args...);
		}
		else if constexpr (std::is_same<Component, MeshComponent>::value)
		{
			return _meshComponent.emplace(args...);
		}
		else static_assert(false, "Unexcepted type passed to the function Emplace() of ComponentList class");
	}

	template<typename Component>
	Component* Get()
	{
		if constexpr (std::is_same<Component, TagComponent>::value)
		{
			if (_tagComponent.has_value())
				return &(*_tagComponent);


			Logger::Log("Trying to get some component which is not initialized! Nullptr returned\n");
			return nullptr;
		}
		else if constexpr (std::is_same<Component, TranslationComponent>::value)
		{
			if (_translationComponent.has_value())
				return &(*_translationComponent);

			Logger::Log("Trying to get some component which is not initialized! Nullptr returned\n");
			return nullptr;
		}
		else if constexpr (std::is_same<Component, CameraComponent>::value)
		{
			if (_cameraComponent.has_value())
				return &(*_cameraComponent);

			Logger::Log("Trying to get some component which is not initialized! Nullptr returned\n");
			return nullptr;
		}
		else if constexpr (std::is_same<Component, MeshComponent>::value)
		{
			if (_meshComponent.has_value())
				return &(*_meshComponent);

			Logger::Log("Trying to get some component which is not initialized! Nullptr returned\n");
			return nullptr;
		}
		else static_assert(false, "Unexcepted type passed to the function Get() of ComponentList class");
	}

};