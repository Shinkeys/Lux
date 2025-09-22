#include "../../headers/scene/scene_base.h"
#include "../../headers/base/core/renderer.h"
#include "../../headers/scene/scene_storage.h"
#include "../../headers/scene/scene_renderer.h"
#include "../../headers/scene/RT_scene_renderer.h"
#include "../../headers/scene/entity.h"

#include <glm/gtc/matrix_transform.hpp>

SceneBase::SceneBase(SceneRenderer& renderer, RTSceneRenderer& rtRenderer, SceneStorage& storage) 
	: _rendererInstance{ renderer }, _rtRendererInstance{ rtRenderer }, _storageInstance { storage }
{
	_camera = std::make_shared<Camera>();
	Initialize();
}


ComponentList* SceneBase::GetComponentListByEntity(const Entity& entity)
{
	return _storageInstance.GetComponentListByEntity(entity);
}



void SceneBase::Initialize()
{
	constexpr bool cameraIsActive = true;

	const Entity& sponza = _storageInstance.CreateEntityInRegistry(this);
	sponza.AddComponent<TagComponent>("Sponza");
	sponza.AddComponent<CameraComponent>(_camera, cameraIsActive);
	sponza.AddComponent<MeshComponent>();
	//sponza.AddComponent<TranslationComponent>(glm::mat4(1.0f), glm::vec3(0.0f), glm::vec3(0.01f));
	glm::mat4 transform = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));
	sponza.AddComponent<TransformComponent>(transform);
	sponza.GetComponent<MeshComponent>()->folderName = "Sponza";

	/*const Entity& wall1 = _storageInstance.CreateEntityInRegistry(this);
	wall1.AddComponent<TagComponent>("Wall1");
	wall1.AddComponent<CameraComponent>(_camera, cameraIsActive);
	wall1.AddComponent<MeshComponent>();
	glm::mat4 transform = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));
	transform = glm::rotate(transform, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	transform = glm::translate(transform, glm::vec3(0.0f, 200.0f, 0.0f));
	wall1.AddComponent<TransformComponent>(transform);
	wall1.GetComponent<MeshComponent>()->folderName = "Wall1";

	const Entity& wall2 = _storageInstance.CreateEntityInRegistry(this);
	wall2.AddComponent<TagComponent>("Wall2");
	wall2.AddComponent<CameraComponent>(_camera, cameraIsActive);
	wall2.AddComponent<MeshComponent>();
	glm::mat4 wall2Transform = glm::scale(glm::mat4(1.0f), glm::vec3(0.4f));
	wall2Transform = glm::translate(wall2Transform, glm::vec3(0.0f, -0.5f, -5.0f));
	wall2.AddComponent<TransformComponent>(wall2Transform);
	wall2.GetComponent<MeshComponent>()->folderName = "Wall2";*/


	_rtRendererInstance.SubmitEntityToDraw(sponza);
	//_rtRendererInstance.SubmitEntityToDraw(wall1);
	//_rtRendererInstance.SubmitEntityToDraw(wall2);
}


void SceneBase::Update()
{
	for (const auto& [ett, comps] : _storageInstance.GetRegistry())
	{
		//_rendererInstance.SubmitEntityToDraw(ett);
	}	
}

void SceneBase::UpdateWithKeys(const Window& window)
{
	_camera->Update(window);
}