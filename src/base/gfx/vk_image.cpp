#include "../../../headers/base/gfx/vk_image.h"
#include "../../../headers/base/gfx/vk_buffer.h"	
#include "../../../headers/base/gfx/vk_deleter.h"
#include "../../../headers/base/gfx/vk_allocator.h"
#include "../../../headers/util/gfx/vk_helpers.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

VulkanImage::VulkanImage(const ImageSpecification& spec, VulkanDevice& deviceObj, VulkanFrame& frameObj, VulkanAllocator& allocatorObj) : 
	_specification{ spec }, _deviceObject { &deviceObj }, _frameObject{ &frameObj }, _allocatorObject{ &allocatorObj }
{
	switch (spec.type)
	{
	case ImageType::IMAGE_TYPE_NONE:
	{
		assert(false && "Unable to create texture, it has undefined type(IMAGE_TYPE_NONE)");
		return;
	}

	case ImageType::IMAGE_TYPE_TEXTURE:
	{
		CreateTexture();
		break;
	}

	case ImageType::IMAGE_TYPE_RENDER_TARGET:
	{
		CreateRenderTarget();
		break;
	}

	case ImageType::IMAGE_TYPE_DEPTH_BUFFER:
		CreateRenderTarget();
		break;

	case ImageType::IMAGE_TYPE_SWAPCHAIN:
	{

	}

	default:
		std::unreachable();
	}
}

VulkanImage::VulkanImage(const ImageSpecification& spec, VkImage image, VkImageView imageView) : _specification{spec}, _image{image}, _imageView{imageView}
{

}


VulkanImage::~VulkanImage()
{

	// Means that image is created manually, without base class. should be deallocated manually
	if (_allocatorObject == nullptr)
		return;

	VkDevice device = _deviceObject->GetDevice();
	VkImageView imgView = _imageView;
	VkImage img = _image;
	VmaAllocator alloc = _allocatorObject->GetAllocatorHandle();
	VmaAllocation allocation = _allocation;

	VulkanDeleter::SubmitObjectDesctruction([device, img, imgView, alloc, allocation]()
	{
		vkDestroyImageView(device, imgView, nullptr);
		vmaDestroyImage(alloc, img, allocation);
	});
}

void VulkanImage::CreateTexture()
{
	stbi_set_flip_vertically_on_load(true);

	int texWidth, texHeight, texChannels;

	stbi_uc* pixels = stbi_load(_specification.path.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
	{
		std::cout << "Failed to load image by path: " << _specification.path << '\n';
		return;
	}

	void* pixelsVoid = pixels;

	void* const& data = nullptr;

	const size_t imageSize = texWidth * texHeight * 4; // 4 bytes

	BufferSpecification spec{};
	spec.usage = BufferUsage::TRANSFER_DST | BufferUsage::TRANSFER_SRC;
	spec.memoryUsage = MemoryUsage::AUTO_PREFER_DEVICE;
	spec.memoryProp = MemoryProperty::DEVICE_LOCAL;
	spec.allocCreate = AllocationCreate::NONE;
	spec.sharingMode = SharingMode::SHARING_EXCLUSIVE;
	spec.size = imageSize;

	assert(_deviceObject && _allocatorObject && _frameObject && "Trying to create a texture with raw created(without base class) vulkan image");

	VulkanBuffer stagingBuffer(spec, *_deviceObject, *_allocatorObject, *_frameObject);
	stagingBuffer.UploadData(0, pixelsVoid, imageSize);

	stbi_image_free(pixels);


	VkExtent3D imageExtent;
	imageExtent.width = static_cast<u32>(texWidth);
	imageExtent.height = static_cast<u32>(texHeight);
	imageExtent.depth = 1;

	_specification.extent = ImageExtent3D{ imageExtent.width, imageExtent.height, imageExtent.depth };

	const u32 mipLevels = static_cast<u32>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	_specification.mipLevels = mipLevels;



	VkFormat imgFormat = vkconversions::ToVkFormat(_specification.format); // SURFACE FORMAT IS BGRA BUT WOULD USE THAT, COULD JUST BLIT IMAGE LATER BEFORE PRESENTATION
	// FOR MORE CONVENIENT APPOACH
	VkImageAspectFlags aspect = vkconversions::ToVkAspectFlags(_specification.aspect);

	VkImageCreateInfo createInfo = vkhelpers::CreateImageInfo(imgFormat, imageExtent, mipLevels, VK_IMAGE_USAGE_SAMPLED_BIT 
		| VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	allocInfo.memoryTypeBits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VK_CHECK(vmaCreateImage(_allocatorObject->GetAllocatorHandle(), &createInfo, &allocInfo, &_image, &_allocation, nullptr));


	VkCommandBuffer cmdBuffer = _frameObject->GetCommandBuffer();


	{
		// Insert barrier for staging buffer
		vkhelpers::InsertMemoryBarrier(cmdBuffer, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);

		// MAKE IMAGE AVAILABLE FOR DATA COPYING. This is only base mip level(0)
		VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = _image;


		barrier.subresourceRange =
		{
			.aspectMask = vkconversions::ToVkAspectFlags(_specification.aspect),
			.baseMipLevel = 0,
			.levelCount = VK_REMAINING_MIP_LEVELS,
			.baseArrayLayer = 0,
			.layerCount = 1
		};

		VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
		dependencyInfo.dependencyFlags = 0;
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &barrier;

		vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);


		VkBufferImageCopy copyRegion{};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		copyRegion.imageSubresource.aspectMask = aspect;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = imageExtent;


		vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.GetRawBuffer(),
			_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);




		int mipWidth = texWidth;
		int mipHeight = texHeight;
		// all others mip levels
		for (u32 i = 1; i < mipLevels; ++i)
		{
			VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
			barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			barrier.image = _image;
			barrier.subresourceRange =
			{
				.aspectMask = vkconversions::ToVkAspectFlags(_specification.aspect),
				.baseMipLevel = i - 1,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			};


			VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
			dependencyInfo.dependencyFlags = 0;
			dependencyInfo.imageMemoryBarrierCount = 1;
			dependencyInfo.pImageMemoryBarriers = &barrier;


			vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);




			VkOffset3D srcOffsets[2];
			VkOffset3D dstOffsets[2];
			srcOffsets[0] = VkOffset3D{ 0,0,0 };
			srcOffsets[1] = VkOffset3D{ mipWidth, mipHeight, 1 };
			dstOffsets[0] = VkOffset3D{ 0,0,0 };
			dstOffsets[1] = VkOffset3D{ mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };

			VkImageBlit2 imgBlit{ VK_STRUCTURE_TYPE_IMAGE_BLIT_2 };
			imgBlit.srcOffsets[0] = srcOffsets[0];
			imgBlit.srcOffsets[1] = srcOffsets[1];
			imgBlit.dstOffsets[0] = dstOffsets[0];
			imgBlit.dstOffsets[1] = dstOffsets[1];
			imgBlit.srcSubresource.mipLevel = i - 1;
			imgBlit.srcSubresource.baseArrayLayer = 0;
			imgBlit.srcSubresource.layerCount = 1;
			imgBlit.dstSubresource.mipLevel = i;
			imgBlit.dstSubresource.baseArrayLayer = 0;
			imgBlit.dstSubresource.layerCount = 1;
			imgBlit.srcSubresource.aspectMask = vkconversions::ToVkAspectFlags(_specification.aspect);
			imgBlit.dstSubresource.aspectMask = vkconversions::ToVkAspectFlags(_specification.aspect);


			VkBlitImageInfo2 blitInfo{ VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2 };
			blitInfo.srcImage = _image;
			blitInfo.dstImage = _image;
			blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			blitInfo.filter = VK_FILTER_LINEAR;
			blitInfo.pRegions = &imgBlit;
			blitInfo.regionCount = 1;


			vkCmdBlitImage2(cmdBuffer, &blitInfo);


			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
			barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

			dependencyInfo.dependencyFlags = 0;
			dependencyInfo.imageMemoryBarrierCount = 1;
			dependencyInfo.pImageMemoryBarriers = &barrier;

			vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		// Transit last mip level
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		barrier.image = _image;
		barrier.subresourceRange.baseMipLevel = mipLevels - 1;

		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &barrier;


		vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
	}


	{
		// Transit image again to make it readable for the shader
		VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = _image;

		barrier.subresourceRange =
		{
			.aspectMask = vkconversions::ToVkAspectFlags(_specification.aspect),
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		};

		VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
		dependencyInfo.dependencyFlags = 0;
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &barrier;

		vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
	}


	VkImageViewCreateInfo imgViewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imgViewCreateInfo.image = _image;
	imgViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imgViewCreateInfo.format = imgFormat;
	imgViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imgViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imgViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imgViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imgViewCreateInfo.subresourceRange.aspectMask = aspect;
	imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imgViewCreateInfo.subresourceRange.layerCount = 1;
	imgViewCreateInfo.subresourceRange.levelCount = mipLevels;

	VK_CHECK(vkCreateImageView(_deviceObject->GetDevice(), &imgViewCreateInfo, nullptr, &_imageView));
}


void VulkanImage::SetLayout(ImageLayout newLayout, AccessFlag srcAccess, AccessFlag dstAccess, PipelineStage srcStage, PipelineStage dstStage)
{
	// Transit image again to make it readable for the shader
	VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
	barrier.oldLayout     = vkconversions::ToVkImageLayout(_specification.layout);
	barrier.newLayout     = vkconversions::ToVkImageLayout(newLayout);
	barrier.srcAccessMask = vkconversions::ToVkAccessFlags2(srcAccess);
	barrier.dstAccessMask = vkconversions::ToVkAccessFlags2(dstAccess);
	barrier.srcStageMask  = vkconversions::ToVkPipelineStageFlags2(srcStage);
	barrier.dstStageMask  = vkconversions::ToVkPipelineStageFlags2(dstStage);

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = _image;

	barrier.subresourceRange =
	{
		.aspectMask = vkconversions::ToVkAspectFlags(_specification.aspect),
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1
	};

	VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	dependencyInfo.dependencyFlags = 0;
	dependencyInfo.imageMemoryBarrierCount = 1;
	dependencyInfo.pImageMemoryBarriers = &barrier;

	vkCmdPipelineBarrier2(_frameObject->GetCommandBuffer(), &dependencyInfo);
}

// TO REWORK THIS CLASS TO RETURN SHARED PTRS WITH CUSTOM ALLOCATOR
// Create image without data
void VulkanImage::CreateRenderTarget()
{
	VkImageCreateInfo createInfo = vkhelpers::CreateImageInfo(vkconversions::ToVkFormat(_specification.format), 
		vkconversions::ToVkExtent3D(_specification.extent), _specification.mipLevels,
		vkconversions::ToVkImageUsage(_specification.usage));

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	allocInfo.memoryTypeBits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VK_CHECK(vmaCreateImage(_allocatorObject->GetAllocatorHandle(), &createInfo, &allocInfo, &_image, &_allocation, nullptr));


	VkImageViewCreateInfo imgViewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imgViewCreateInfo.image = _image;
	imgViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imgViewCreateInfo.format = vkconversions::ToVkFormat(_specification.format);
	imgViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imgViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imgViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imgViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imgViewCreateInfo.subresourceRange.aspectMask = vkconversions::ToVkAspectFlags(_specification.aspect);
	imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imgViewCreateInfo.subresourceRange.layerCount = 1;
	imgViewCreateInfo.subresourceRange.levelCount = 1;

	VK_CHECK(vkCreateImageView(_deviceObject->GetDevice(), &imgViewCreateInfo, nullptr, &_imageView));

}

/*
 .|'''.|      |     '||    ||' '||''|.  '||'      '||''''|  '||''|.
 ||..  '     |||     |||  |||   ||   ||  ||        ||  .     ||   ||
  ''|||.    |  ||    |'|..'||   ||...|'  ||        ||''|     ||''|'
.     '||  .''''|.   | '|' ||   ||       ||        ||        ||   |.
|'....|'  .|.  .||. .|. | .||. .||.     .||.....| .||.....| .||.  '|'

*/

VulkanSampler::VulkanSampler(const SamplerSpecification& spec, VulkanDevice& deviceObject) : _specification{spec}, _deviceObject{deviceObject}
{
	const float anisotropyLevel = _deviceObject.GetMaxAnisotropyLevel();

	VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerInfo.magFilter    = vkconversions::ToVkFilter(spec.magFiltering);
	samplerInfo.minFilter    = vkconversions::ToVkFilter(spec.minFiltering);
	samplerInfo.mipmapMode   = vkconversions::ToVkMipmapMode(spec.mipmapFiltering);
	samplerInfo.addressModeU = vkconversions::ToVkAddressMode(spec.addressMode);
	samplerInfo.addressModeV = vkconversions::ToVkAddressMode(spec.addressMode);
	samplerInfo.addressModeW = vkconversions::ToVkAddressMode(spec.addressMode);
	samplerInfo.anisotropyEnable = VK_FALSE; //anisotropyLevel > 1.0f ? VK_TRUE : VK_FALSE;
	samplerInfo.maxAnisotropy = anisotropyLevel;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.minLod = spec.minLod;
	samplerInfo.maxLod = spec.maxLod;
	samplerInfo.mipLodBias = spec.lodBias;

	VK_CHECK(vkCreateSampler(_deviceObject.GetDevice(), &samplerInfo, nullptr, &_sampler));
}

VulkanSampler::~VulkanSampler()
{
	VkDevice device = _deviceObject.GetDevice();
	VkSampler sampler = _sampler;
	VulkanDeleter::SubmitObjectDesctruction([device, sampler]() {
		vkDestroySampler(device, sampler, nullptr);
	});
}





namespace vkconversions
{
	VkFormat ToVkFormat(ImageFormat format) 
	{
		switch (format) {
		case ImageFormat::IMAGE_FORMAT_R8G8B8A8_SRGB:           return VK_FORMAT_R8G8B8A8_SRGB;
		case ImageFormat::IMAGE_FORMAT_B8G8R8A8_SRGB:           return VK_FORMAT_B8G8R8A8_SRGB;
		case ImageFormat::IMAGE_FORMAT_D32_SFLOAT:              return VK_FORMAT_D32_SFLOAT;
		case ImageFormat::IMAGE_FORMAT_R16G16B16A16_SFLOAT:     return VK_FORMAT_R16G16B16A16_SFLOAT;
		case ImageFormat::IMAGE_FORMAT_R32G32_UINT:             return VK_FORMAT_R32G32_UINT;
		default: std::unreachable();
		}
	}

	VkImageUsageFlags ToVkImageUsage(ImageUsage usage) 
	{
		VkImageUsageFlags result = 0;

		if (usage & ImageUsage::IMAGE_USAGE_COLOR_ATTACHMENT)
			result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		if (usage & ImageUsage::IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT)
			result = result | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		if (usage & ImageUsage::IMAGE_USAGE_SAMPLED)
			result |= VK_IMAGE_USAGE_SAMPLED_BIT;

		if (usage & ImageUsage::IMAGE_USAGE_STORAGE_BIT)
			result |= VK_IMAGE_USAGE_STORAGE_BIT;

		assert(result > 0 && "Some ImageUsage conversion is not implemented for Vulkan");

		return result;
	}

	VkImageLayout ToVkImageLayout(ImageLayout layout) 
	{
		switch (layout) {
		case ImageLayout::IMAGE_LAYOUT_UNDEFINED:                           return VK_IMAGE_LAYOUT_UNDEFINED;
		case ImageLayout::IMAGE_LAYOUT_GENERAL:                             return VK_IMAGE_LAYOUT_GENERAL;
		case ImageLayout::IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case ImageLayout::IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:            return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		case ImageLayout::IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case ImageLayout::IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:     return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		case ImageLayout::IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case ImageLayout::IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:                return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		case ImageLayout::IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:                return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case ImageLayout::IMAGE_LAYOUT_PREINITIALIZED:                      return VK_IMAGE_LAYOUT_PREINITIALIZED;
		default:                                                            std::unreachable();
		}
	}

	VkImageAspectFlags ToVkAspectFlags(ImageAspect aspect) 
	{
		switch (aspect) {
		case ImageAspect::IMAGE_ASPECT_COLOR: return VK_IMAGE_ASPECT_COLOR_BIT;
		case ImageAspect::IMAGE_ASPECT_DEPTH: return VK_IMAGE_ASPECT_DEPTH_BIT;
		default: std::unreachable();
		}
	}

	VkFilter ToVkFilter(Filter filter) 
	{
		switch (filter) {
		case Filter::FILTER_LINEAR: return VK_FILTER_LINEAR;
		case Filter::FILTER_NEAREST:   return VK_FILTER_NEAREST;
		default: std::unreachable();
		}
	}

	VkSamplerMipmapMode ToVkMipmapMode(SamplerMipMapMode mode) 
	{
		switch (mode) {
		case SamplerMipMapMode::SAMPLER_MIPMAP_MODE_LINEAR:  return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		case SamplerMipMapMode::SAMPLER_MIPMAP_MODE_NEAREST: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		default: std::unreachable();
		}
	}

	VkSamplerAddressMode ToVkAddressMode(SamplerAddressMode mode) 
	{
		switch (mode) {
		case SamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		default: std::unreachable();
		}
	}

	VkExtent2D ToVkExtent2D(ImageExtent2D extent) 
	{
		return VkExtent2D{ extent.x, extent.y };
	}

	VkExtent3D ToVkExtent3D(ImageExtent3D extent) 
	{
		return VkExtent3D{ extent.x, extent.y, extent.z };
	}

	ImageExtent2D ToEngineExtent2D(VkExtent2D extent)
	{
		return ImageExtent2D{ extent.width, extent.height };
	}

	ImageExtent3D ToEngineExtent3D(VkExtent3D extent)
	{
		return ImageExtent3D{ extent.width, extent.height, extent.depth };
	}


	ImageFormat ToEngineFormat(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_R8G8B8A8_SRGB:
			return ImageFormat::IMAGE_FORMAT_R8G8B8A8_SRGB;
		case VK_FORMAT_B8G8R8A8_SRGB:
			return ImageFormat::IMAGE_FORMAT_B8G8R8A8_SRGB;
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			return ImageFormat::IMAGE_FORMAT_R16G16B16A16_SFLOAT;
		case VK_FORMAT_D32_SFLOAT:
			return ImageFormat::IMAGE_FORMAT_D32_SFLOAT;
		case VK_FORMAT_R32G32_UINT:
			return ImageFormat::IMAGE_FORMAT_R32G32_UINT;

		default: std::unreachable();
		}
	}
}