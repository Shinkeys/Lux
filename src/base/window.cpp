#include "../../headers/base/window.h"

bool Window::Initialize()
{
	SDL_Init(SDL_INIT_VIDEO);

	const SDL_DisplayMode* dM = SDL_GetCurrentDisplayMode(1);

	if (dM == nullptr)
	{
		std::cout << "Unable to initialize window. Error: " << SDL_GetError() << '\n';
		return false;
	}

	_width = dM->w;
	_height = dM->h;
	
	
	_window = SDL_CreateWindow(_title, _width, _height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

	if (_window == nullptr)
	{
		std::cout << "Unable to initialize window. Error: " << SDL_GetError() << '\n';
		return false;
	}

	return true;
}


void Window::Update()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_EVENT_WINDOW_RESIZED)
		{
			SDL_GetWindowSize(_window, &_width, &_height);
		}


		if (event.type == SDL_EVENT_QUIT)
			_closeWindow = true;

		if (event.type == SDL_EVENT_KEY_DOWN)
		{
			switch (event.key.scancode)
			{
			case SDL_SCANCODE_ESCAPE:
				_closeWindow = true;
				break;

			default:
				break;
			}
		}
	}
}


void Window::Cleanup()
{
	
	SDL_DestroyWindow(_window);
	SDL_Quit();
}
