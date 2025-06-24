#include "../../headers/base/window.h"

float Window::_deltaTime{ 0.0f };

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


	_mouse.lastPosX = static_cast<float>(_width) / 2.0f;
	_mouse.lastPosY = static_cast<float>(_height) / 2.0f;

	_window = SDL_CreateWindow(_title, _width, _height, SDL_WINDOW_VULKAN);
	SDL_SetWindowFullscreen(_window, true);
	SDL_HideCursor();

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
	_mouse.xOffset = 0.0f;
	_mouse.yOffset = 0.0f;


	static float lastFrame = _deltaTime;
	_deltaTime = (SDL_GetTicks() / 1000.0f) - lastFrame;
	lastFrame = SDL_GetTicks() / 1000.0f;


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

			// temp
			_keys[event.key.scancode] = true;
		}

		if (event.type == SDL_EVENT_KEY_UP)
		{
			_keys[event.key.scancode] = false;
		}

		if (event.type == SDL_EVENT_MOUSE_MOTION)
		{
			SDL_WarpMouseInWindow(_window, _width / 2.0f, _height / 2.0f); // to make mouse rotate by 360
			
			
			_mouse.xOffset = _mouse.lastPosX - event.motion.x;
			_mouse.yOffset = _mouse.lastPosY - event.motion.y;

			_mouse.lastPosX = event.motion.x;
			_mouse.lastPosY = event.motion.y;
		}
	}
}


void Window::Cleanup()
{
	
	SDL_DestroyWindow(_window);
	SDL_Quit();
}
