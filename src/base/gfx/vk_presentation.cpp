#include "../../../headers/base/gfx/vk_presentation.h"
#include "../../../headers/base/gfx/vk_frame.h"
#include "../../../headers/base/gfx/vk_buffer.h"
#include "../../../headers/base/gfx/vk_image.h"
#include "../../../headers/base/gfx/vk_deleter.h"

u32 VulkanPresentation::PresentationImagesCount{ 0 };

VulkanPresentation::VulkanPresentation(VulkanInstance& instanceObj, VulkanDevice& deviceObj, Window& windowObj) :
	_instanceObject{ instanceObj }, _deviceObject{ deviceObj }, _windowObject { windowObj }
{
	CreateSwapchain();
}

void VulkanPresentation::RecreateSwapchain()
{
	vkDeviceWaitIdle(_deviceObject.GetDevice());

	DestroyStructures();
	CreateSwapchain();
}
void VulkanPresentation::CreateSwapchain()
{
	const VkPhysicalDevice physDevice = _deviceObject.GetPhysicalDevice();
	const VkSurfaceKHR surface = _instanceObject.GetSurface();

	VkSurfaceCapabilitiesKHR surfaceCaps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &surfaceCaps);


	VkSurfaceFormatKHR surfaceFormat = FindRequiredSurfaceFormat();
	VkPresentModeKHR presentMode = FindRequiredPresentMode();
	VkExtent2D swapchainExtent = SelectRequiredSwapchainExtent(surfaceCaps);

	// 4 should be enough
	PresentationImagesCount = std::min(std::max(4u, surfaceCaps.minImageCount), surfaceCaps.maxImageCount);


	VkSwapchainCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	createInfo.pNext = nullptr;
	createInfo.surface = surface;
	createInfo.minImageCount = PresentationImagesCount;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageExtent = swapchainExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	// Means that image explicitly should be transferred before using in another queue family
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.pQueueFamilyIndices = nullptr;
	// Change it if want to rotate images in the swapchain
	createInfo.preTransform = surfaceCaps.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	// To do
	createInfo.oldSwapchain = _swapchainDesc.swapchain;

	VK_CHECK(vkCreateSwapchainKHR(_deviceObject.GetDevice(), &createInfo, nullptr, &_swapchainDesc.swapchain));

	Logger::Log("Vulkan create swapchain", _swapchainDesc.swapchain, LogLevel::Fatal);

	u32 swapImageCount;
	vkGetSwapchainImagesKHR(_deviceObject.GetDevice(), _swapchainDesc.swapchain, &swapImageCount, nullptr);

	if (_swapchainDesc.images.empty())
		_swapchainDesc.images.resize(swapImageCount);

	// Need to do it like that because ImageHandle is a shared pointers
	std::vector<VkImage> images(_swapchainDesc.images.size());

	vkGetSwapchainImagesKHR(_deviceObject.GetDevice(), _swapchainDesc.swapchain, &swapImageCount, images.data());

	ImageSpecification imageSpec;
	imageSpec.aspect = ImageAspect::IMAGE_ASPECT_COLOR;
	imageSpec.format = vkconversions::ToEngineFormat(surfaceFormat.format);
	imageSpec.extent = ImageExtent3D{ swapchainExtent.width, swapchainExtent.height, 1 };

	for (u32 i = 0; i < _swapchainDesc.images.size(); ++i)
	{
		if (_swapchainDesc.images[i] == nullptr)
		{
			VkImageViewCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			createInfo.image = images[i];
			createInfo.format = surfaceFormat.format;
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			// Describes image purpose. Default color attachment for now
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.layerCount = 1;

			VkImageView imageView;
			VK_CHECK(vkCreateImageView(_deviceObject.GetDevice(), &createInfo, nullptr, &imageView));

			_swapchainDesc.images[i] = std::make_shared<VulkanImage>(imageSpec, images[i], imageView);
		}
	}

	_swapchainDesc.imageFormat = surfaceFormat.format;
	_swapchainDesc.extent = swapchainExtent;
}

VkSurfaceFormatKHR VulkanPresentation::FindRequiredSurfaceFormat() const
{
	const VkPhysicalDevice physDevice = _deviceObject.GetPhysicalDevice();
	const VkSurfaceKHR surface = _instanceObject.GetSurface();

	u32 formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, nullptr);

	std::vector<VkSurfaceFormatKHR> formats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, formats.data());

	auto formatIt = std::find_if(formats.begin(), formats.end(), [this](VkSurfaceFormatKHR format)
		{
			return format.format == ColorFormat.format && format.colorSpace == ColorFormat.colorSpace;
		});

	Logger::Log("Vulkan surface format selection", formatIt != formats.end(), LogLevel::Debug);

	return *formatIt;
}

VkPresentModeKHR VulkanPresentation::FindRequiredPresentMode() const
{
	VkPhysicalDevice physDevice = _deviceObject.GetPhysicalDevice();
	VkSurfaceKHR surface = _instanceObject.GetSurface();

	u32 presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, nullptr);

	std::vector<VkPresentModeKHR> availablePresentModes(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, availablePresentModes.data());

	std::vector<VkPresentModeKHR> desiredPresentModes
	{
		VK_PRESENT_MODE_MAILBOX_KHR
	};

	// FIFO is guaranteed so no need to assert
	VkPresentModeKHR selectedPresentMode{ VK_PRESENT_MODE_FIFO_KHR };
	for (VkPresentModeKHR mode : availablePresentModes)
	{
		if (std::find(desiredPresentModes.begin(), desiredPresentModes.end(), mode) != desiredPresentModes.end())
		{
			selectedPresentMode = mode;
			break;
		}
	}

	Logger::Log("Vulkan present mode", selectedPresentMode);

	return selectedPresentMode;
}


VkExtent2D VulkanPresentation::SelectRequiredSwapchainExtent(const VkSurfaceCapabilitiesKHR& caps) const
{
	if (caps.currentExtent.width != std::numeric_limits<u32>::max())
	{
		Logger::Log("Vulkan extent 2D", caps.currentExtent);
		return caps.currentExtent;
	}

	i32 width;
	i32 height;

	SDL_GetWindowSizeInPixels(_windowObject.GetWindowPtr(), &width, &height);

	VkExtent2D extent
	{
		static_cast<u32>(width),
		static_cast<u32>(height)
	};

	extent.width  = std::clamp(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
	extent.height = std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);

	Logger::Log("Vulkan extent 2D", extent);
	


	return extent;
}


void VulkanPresentation::Cleanup()
{
	DestroyStructures();
	DestroySwapchain();
}

void VulkanPresentation::DestroyStructures()
{
	for (auto& imageHandle : _swapchainDesc.images)
	{
		if (imageHandle != nullptr)
		{
			vkDestroyImageView(_deviceObject.GetDevice(), imageHandle->GetRawView(), nullptr);
		}
	}
}

void VulkanPresentation::DestroySwapchain()
{
	VkDevice device = _deviceObject.GetDevice();
	VkSwapchainKHR swapchain = _swapchainDesc.swapchain;
	VulkanDeleter::SubmitObjectDesctruction([device, swapchain]() {
		vkDestroySwapchainKHR(device, swapchain, nullptr);
	});
}