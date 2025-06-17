#pragma once
#include "../../util/util.h"
#include "../../util/logger.h"
#include "../../util/gfx/vk_types.h"

class Window;
class VulkanInstance
{
private:
	Window& _windowObject;
	VkInstance _instance;
	VkSurfaceKHR _surface;
	VkDebugUtilsMessengerEXT _debugMessenger;

	void CreateInstance();
	void CreateDebugMessenger();
	bool IsLayerSupported(const char* name);
	bool IsInstanceExtensionSupported(const char* name);
	void FillDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	void CreateSurface();
public:
	VulkanInstance(Window& window);
	void Cleanup();
	VkInstance GetInstance()  const { return _instance; }
	VkSurfaceKHR GetSurface() const { return _surface;  }
};