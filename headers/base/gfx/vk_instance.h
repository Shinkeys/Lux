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
	VulkanInstance() = delete;
	~VulkanInstance() = default;
	VulkanInstance(Window& window);

	VulkanInstance(const VulkanInstance&) = delete;
	VulkanInstance(VulkanInstance&&) = delete;
	VulkanInstance& operator= (const VulkanInstance&) = delete;
	VulkanInstance& operator= (VulkanInstance&&) = delete;


	VkInstance GetInstance()  const { return _instance; }
	VkSurfaceKHR GetSurface() const { return _surface;  }

	void Cleanup();
};