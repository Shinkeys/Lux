#pragma once
#include "../util/util.h"
#include "gfx/vk_instance.h"

#include <SDL3/SDL.h>

class Window
{
private:
	i32 _width { 0 };
	i32 _height{ 0 };
	SDL_Window* _window = nullptr;
	const char* _title = "Lux";

	bool _closeWindow{ false };

public:
	bool Initialize();
	void Cleanup();
	void Update();
	bool WindowShouldClose() const { return _closeWindow; }
	
	SDL_Window* GetWindowPtr() const { return _window; }
};