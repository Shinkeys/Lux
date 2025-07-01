#pragma once
#include "../util.h"
#include <volk.h>


struct VulkanSwapchain
{
	std::vector<VkImage> images;
	std::vector<VkImageView> imagesView;
	VkSwapchainKHR swapchain{ VK_NULL_HANDLE };
	VkFormat imageFormat{ VK_FORMAT_UNDEFINED };
	VkExtent2D extent{ 0,0 };
};



struct SSBOPair
{
	VkDeviceAddress address{ 0 };
	i32 index{ -1 };
};

class StorageBuffer;
struct StagingPair
{
	StorageBuffer* buffer{ nullptr };
	i32 index{ -1 };
};
