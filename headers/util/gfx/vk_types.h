#pragma once
#include "../util.h"
#include <volk.h>

#include <vk_mem_alloc.h>

struct SSBOPair
{
	VkDeviceAddress address{ 0 };
	i32 index{ -1 };
};

struct UBOPair
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



// IMAGES

enum class VulkanImageUsage : u8
{
	USAGE_RENDER_TARGET,
	USAGE_DEPTH_TARGET,
	USAGE_STORAGE_BIT,
};

class VulkanImage;
struct VulkanSwapchain
{
	std::vector<std::shared_ptr<VulkanImage>> images;
	VkSwapchainKHR swapchain{ VK_NULL_HANDLE };
	VkFormat imageFormat{ VK_FORMAT_UNDEFINED };
	VkExtent2D extent{ 0,0 };
};
