#pragma once
#include "vk_presentation.h"
#include "../../util/gfx/vk_types.h"
#include "../../asset/asset_types.h"

struct ImageSpecification
{
	VkFormat format{ VulkanPresentation::ColorFormat.format };
	VkExtent3D extent{ 0, 0 };
	VkImageUsageFlags usage{ VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT };
	VkImageLayout newLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
	VkImageAspectFlags aspect{ VK_IMAGE_ASPECT_COLOR_BIT };
	u32 mipLevels{ 1 };
};
// 


class VulkanBuffer;
class VulkanAllocator;
class VulkanFrame;
class VulkanDevice;
struct MeshMaterial;

class VulkanImage
{
private:
	VulkanBuffer& _bufferObject;
	VulkanFrame& _frameObject;
	VulkanAllocator& _allocatorObject;
	VulkanDevice& _deviceObject;

	std::vector<VkSampler> _allSamplersStorage;

	std::vector<ImageHandle> _allLoadedImagesStorage; // global storage of all loaded images
	std::vector<ImageHandle> _allCreatedImagesStorage; // global storage for all created images
	using HandleIndex = u32;
	HandleIndex _imageAvailableIndex{ 1 };

	std::queue<LayoutsUpdateDesc> _queueToChangeLayouts;

	/**
	* @brief Change images layout when cmd buffer would start recording. ITS impossible to do it when image is created
	*/
	void CreateImageView(ImageHandle& imgHandle, VkFormat format, VkImageAspectFlags aspectMask);
public:
	void Cleanup();
	void UpdateLayoutsToCopyData();
	VulkanImage() = delete;
	~VulkanImage() = default;
	VulkanImage(VulkanDevice& deviceObject, VulkanBuffer& bufferObj, VulkanFrame& frameObj, VulkanAllocator& allocatorObj);


	VulkanImage(const VulkanImage&) = delete;
	VulkanImage(VulkanImage&&) = delete;
	VulkanImage& operator= (const VulkanImage&) = delete;
	VulkanImage& operator= (VulkanImage&&) = delete;

	VkSampler CreateSampler(const CreateSamplerSpec& spec);
	ImageHandle* LoadAndStoreImageFromFile(const fs::path& path);
	ImageHandle& CreateEmptyImage(const ImageSpecification& spec);
	const std::vector<ImageHandle>& GetAllLoadedImages() const { return _allLoadedImagesStorage; }
};