#include "../../headers/scene/scene_base.h"
#include "../../headers/scene/scene_storage.h"
#include "../../headers/scene/scene_renderer.h"
#include "../../headers/base/core/renderer.h"
#include "../../headers/scene/entity.h"


SceneBase::SceneBase(SceneRenderer& renderer, SceneStorage& storage) : _rendererInstance{ renderer }, _storageInstance{storage}
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
	sponza.AddComponent<TranslationComponent>(glm::mat4(1.0f), glm::vec3(0.0f), glm::vec3(0.001f));
	MeshComponent* sponzaMeshComp = sponza.GetComponent<MeshComponent>();
	sponzaMeshComp->folderName = "Sponza";
}


void SceneBase::Update()
{
	for (const auto& [ett, comps] : _storageInstance.GetRegistry())
	{
		_rendererInstance.SubmitEntityToDraw(ett);
	}
}

void SceneBase::UpdateWithKeys(const Window& window)
{
	_camera->Update(window);
}