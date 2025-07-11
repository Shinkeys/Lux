#pragma once
#include "vk_device.h"
#include "../window.h"


class VulkanDevice;
class VulkanBuffer;
class VulkanAllocator;
class VulkanFrame;
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
	void DestroySwapchain();
	void DestroyStructures();
public:
	static u32 PresentationImagesCount;

	const VulkanSwapchain& GetSwapchainDesc() const { return _swapchainDesc; }
	void RecreateSwapchain();

	VulkanPresentation() = delete;
	~VulkanPresentation() = default;
	VulkanPresentation(VulkanInstance& instanceObj, VulkanDevice& deviceObj, Window& windowObj);


	VulkanPresentation(const VulkanPresentation&) = delete;
	VulkanPresentation(VulkanPresentation&&) = delete;
	VulkanPresentation& operator= (const VulkanPresentation&) = delete;
	VulkanPresentation& operator= (VulkanPresentation&&) = delete;

	void Cleanup();

	static constexpr VkSurfaceFormatKHR ColorFormat{
	.format {VK_FORMAT_B8G8R8A8_SRGB}, .colorSpace{VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
	};

};