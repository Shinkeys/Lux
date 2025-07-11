#include "../../headers/base/application.h"


void Application::Update()
{
	_core.Update();
}

void Application::Run()
{
	if (!_window.Initialize())
	{
		std::cout << "Terminate program. Error: Can't create window system\n";
		return;
	}

	if (!_core.Initialize())
	{
		std::cout << "Terminate program. Error: Can't create core of the engine\n";
		return;
	}

	_vulkanBackend.Initialize(_window);

	_engineBase = std::make_unique<EngineBase>(_vulkanBackend);

	_sceneManager = std::make_unique<SceneManager>(_vulkanBackend, *_engineBase, _window);

	AssetManager::Initialize();
	Renderer::Initialize(_vulkanBackend);

	while (!_window.WindowShouldClose())
	{
		_window.Update();
		Update();
		Renderer::BeginFrame();

		this->Render();

		Renderer::EndFrame();
	}


	// Application closed, cleanup everything.
	Cleanup();
}

void Application::Render()
{
	// Call every render method here
	_core.Render();
	_sceneManager->Update();
}


void Application::Cleanup()
{
	AssetManager::Cleanup();
	_core.Cleanup();
	_window.Cleanup();
}