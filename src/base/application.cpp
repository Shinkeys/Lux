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

	_sceneManager = std::make_unique<SceneManager>(_vulkanBackend, _window);

	AssetManager::Initialize();

	Renderer::Initialize(_vulkanBackend);

	while (!_window.WindowShouldClose())
	{
		_window.Update();
		Update();
		_sceneManager->Update();
		_vulkanBackend.RenderFrame();


		Render();
	}


	// Application closed, cleanup everything.
	AssetManager::Cleanup();
	Cleanup();
}

void Application::Render()
{
	// Call every render method here
	_core.Render();
}


void Application::Cleanup()
{
	_core.Cleanup();
	_vulkanBackend.Cleanup();
	_window.Cleanup();
}