#pragma once
#include "../../util/gfx/vk_types.h"
#include "../../asset/asset_types.h"

#include <vk_mem_alloc.h>

struct ImageHandle
{
	u32 index{ 0 };
	VkImage image{ VK_NULL_HANDLE };
	VkImageView imageView{ VK_NULL_HANDLE };
	VmaAllocation allocation{ nullptr };
};

//struct MaterialVkHandle
//{
//	u32 index{ 0 };
//	ImageHandle albedoTexture;
//	ImageHandle normalTexture;
//	ImageHandle metallicRoughnessTexture;
//};

struct LayoutsUpdateDesc
{
	StagingPair stagingPair{};	
	ImageHandle imgHandle{};
	VkExtent3D imgExtent{0, 0, 0};
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

	std::vector<ImageHandle> _allImagesStorage; // global storage of all images
	using HandleIndex = u32;
	HandleIndex _imageAvailableIndex{ 1 };

	std::queue<LayoutsUpdateDesc> _queueToChangeLayouts;

	/**
	* @brief Change images layout when cmd buffer would start recording. ITS impossible to do it when image is created
	*/
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
	const std::vector<ImageHandle>& GetAllImages() const { return _allImagesStorage; }
};