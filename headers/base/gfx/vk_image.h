#pragma once
#include "vk_presentation.h"
#include "../../util/gfx/vk_types.h"
#include "../../asset/asset_types.h"
#include "../core/image.h"

namespace vkconversions
{
	VkFormat ToVkFormat(ImageFormat format);
	VkImageUsageFlags ToVkImageUsage(ImageUsage usage);
	VkImageLayout ToVkImageLayout(ImageLayout layout);
	VkImageAspectFlags ToVkAspectFlags(ImageAspect aspect);
	VkFilter ToVkFilter(Filter filter);
	VkSamplerMipmapMode ToVkMipmapMode(SamplerMipMapMode mode);
	VkSamplerAddressMode ToVkAddressMode(SamplerAddressMode mode);
	VkExtent2D ToVkExtent2D(const ImageExtent2D& extent);
	VkExtent3D ToVkExtent3D(const ImageExtent3D& extent);

	ImageFormat ToEngineFormat(VkFormat format);
}


class VulkanBuffer;
class VulkanAllocator;
class VulkanFrame;
class VulkanDevice;
struct MeshMaterial;

class VulkanImage : public Image
{
private:
	VulkanBuffer* _bufferObject{ nullptr };
	VulkanFrame* _frameObject{ nullptr };
	VulkanAllocator* _allocatorObject{ nullptr };
	VulkanDevice* _deviceObject{ nullptr };


	VkImageView _imageView{ VK_NULL_HANDLE };
	VkImage		_image{ VK_NULL_HANDLE };
	VmaAllocation _allocation{ nullptr };

	ImageSpecification _specification;

	/**
	* @brief Change images layout when cmd buffer would start recording. ITS impossible to do it when image is created
	*/
	void CreateTexture();
	void CreateRenderTarget();
public:
	VulkanImage(const ImageSpecification& spec, VulkanDevice& deviceObject, VulkanBuffer& bufferObj, VulkanFrame& frameObj, VulkanAllocator& allocatorObj);
	VulkanImage(const ImageSpecification& spec, VkImage image, VkImageView imageView);
	~VulkanImage();

	// OBJECT MANAGED VIA UNIQUE PTR
	VulkanImage(const VulkanImage&) = delete;
	VulkanImage& operator=(const VulkanImage&) = delete;
	VulkanImage(VulkanImage&&) noexcept = delete;
	VulkanImage& operator=(VulkanImage&&) noexcept = delete;


	void SetLayout(ImageLayout newLayout, AccessFlag srcAccess, AccessFlag dstAccess, PipelineStage srcStage, PipelineStage dstStage) override;
	void SetCurrentLayout(ImageLayout layout) override { _specification.layout = layout; }

	VkImageView GetRawView()  const { return _imageView; }
	VkImage     GetRawImage() const { return _image; }

	const ImageSpecification& GetSpecification() const override { return _specification; }
};

class VulkanSampler : public Sampler
{
private:
	VulkanDevice& _deviceObject;

	VkSampler _sampler{ VK_NULL_HANDLE };
	SamplerSpecification _specification;
public:
	VulkanSampler(const SamplerSpecification& spec, VulkanDevice& deviceObject);
	~VulkanSampler();

	// OBJECT MANAGED VIA UNIQUE PTR
	VulkanSampler(const VulkanSampler&) = delete;
	VulkanSampler& operator=(const VulkanSampler&) = delete;
	VulkanSampler(VulkanSampler&&) noexcept = delete;
	VulkanSampler& operator=(VulkanSampler&&) noexcept = delete;

	VkSampler GetRawSampler() const { return _sampler; }
	const SamplerSpecification& GetSpecification() const { return _specification; }

};