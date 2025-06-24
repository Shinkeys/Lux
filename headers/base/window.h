#pragma once
#include "../util/util.h"
#include "gfx/vk_instance.h"

#include <SDL3/SDL.h>

struct Mouse
{
	float lastPosX;
	float lastPosY;

	float xOffset{ 0.0f };
	float yOffset{ 0.0f };
};

class Window
{
private:
	i32 _width { 0 };
	i32 _height{ 0 };
	SDL_Window* _window = nullptr;
	const char* _title = "Lux";
	Mouse _mouse;

	bool _closeWindow{ false };
	std::array<bool, SDL_SCANCODE_COUNT> _keys{ false };


	static float _deltaTime;
public:
	static float GetDeltaTime() { return _deltaTime; }

	bool Initialize();
	void Cleanup();
	void Update();
	bool WindowShouldClose() const { return _closeWindow; }
	bool GetKeyStatus(SDL_Scancode key) const { return _keys[key]; }
	const Mouse& GetMouseDesc() const { return _mouse; }
	
	SDL_Window* GetWindowPtr() const { return _window; }
};