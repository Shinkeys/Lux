#pragma once
#include "../util.h"
#include <volk.h>

#include <vk_mem_alloc.h>

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



// IMAGES

enum class ImageUsage : u8
{
	USAGE_RENDER_TARGET,
	USAGE_DEPTH_TARGET,

};

struct ImageHandle
{
	u32 index{ 0 };
	VkImage image{ VK_NULL_HANDLE };
	VkImageView imageView{ VK_NULL_HANDLE };
	VmaAllocation allocation{ nullptr };
	ImageUsage usage{ ImageUsage::USAGE_RENDER_TARGET };
};

struct LayoutsUpdateDesc
{
	std::optional<StagingPair> stagingPair{};
	std::shared_ptr<ImageHandle> imgHandle{};
	VkExtent3D imgExtent{ 0, 0, 0 };
	VkImageLayout newLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
	VkImageAspectFlags aspect{ VK_IMAGE_ASPECT_COLOR_BIT };
};

struct CreateSamplerSpec
{
	VkFilter minFiltering{ VK_FILTER_LINEAR };
	VkFilter magFiltering{ VK_FILTER_LINEAR };
	VkSamplerMipmapMode mipmapFiltering{ VK_SAMPLER_MIPMAP_MODE_LINEAR };
	VkSamplerAddressMode addressMode{ VK_SAMPLER_ADDRESS_MODE_REPEAT };
	float minLod{ 0.0f };
	float maxLod{ 100.0f };
	float lodBias{ 0.0f };
};


struct VulkanSwapchain
{
	std::vector<std::shared_ptr<ImageHandle>> images;
	VkSwapchainKHR swapchain{ VK_NULL_HANDLE };
	VkFormat imageFormat{ VK_FORMAT_UNDEFINED };
	VkExtent2D extent{ 0,0 };
};

