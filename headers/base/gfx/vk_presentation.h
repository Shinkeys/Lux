#pragma once
#include "vk_device.h"
#include "../window.h"


class VulkanDevice;
class VulkanPresentation
{
private:
	VulkanInstance& _instanceObject;
	VulkanDevice& _deviceObject;
	Window& _windowObject;

	VulkanSwapchain _swapchainDesc;

	VkSurfaceFormatKHR FindRequiredSurfaceFormat()									   const;
	VkPresentModeKHR FindRequiredPresentMode()										   const;
	VkExtent2D SelectRequiredSwapchainExtent(const VkSurfaceCapabilitiesKHR& caps)     const;
	void CreateSwapchain();
	void CreateImageViews();
	void DestroySwapchain();
	void DestroyStructures();
public:
	const VulkanSwapchain& GetSwapchainDesc() const { return _swapchainDesc; }
	void RecreateSwapchain();

	void Cleanup();
	VulkanPresentation() = delete;
	~VulkanPresentation() = default;
	VulkanPresentation(VulkanInstance& instanceObj, VulkanDevice& deviceObj, Window& windowObj);


	VulkanPresentation(const VulkanPresentation&) = delete;
	VulkanPresentation(VulkanPresentation&&) = delete;
	VulkanPresentation& operator= (const VulkanPresentation&) = delete;
	VulkanPresentation& operator= (VulkanPresentation&&) = delete;


};