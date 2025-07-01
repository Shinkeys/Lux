#include "../../../headers/base/gfx/vk_image.h"
#include "../../../headers/base/gfx/vk_buffer.h"	
#include "../../../headers/util/gfx/vk_helpers.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

VulkanImage::VulkanImage(VulkanDevice& deviceObj, VulkanBuffer& bufferObj, VulkanFrame& frameObj, VulkanAllocator& allocatorObj) : _deviceObject{ deviceObj },
		_bufferObject { bufferObj }, _frameObject{frameObj}, _allocatorObject{allocatorObj}
{
}

VkSampler VulkanImage::CreateSampler(const CreateSamplerSpec& spec)
{
	const float anisotropyLevel = _deviceObject.GetMaxAnisotropyLevel();

	VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerInfo.magFilter = spec.magFiltering;
	samplerInfo.minFilter = spec.minFiltering;
	samplerInfo.mipmapMode = spec.mipmapFiltering;
	samplerInfo.addressModeU = spec.addressMode;
	samplerInfo.addressModeV = spec.addressMode;
	samplerInfo.addressModeW = spec.addressMode;
	samplerInfo.anisotropyEnable = anisotropyLevel == 1.0f ? VK_TRUE : VK_FALSE;
	samplerInfo.maxAnisotropy = anisotropyLevel;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.minLod = spec.minLod;
	samplerInfo.maxLod = spec.maxLod;
	samplerInfo.mipLodBias = spec.lodBias;

	VkSampler sampler;

	VK_CHECK(vkCreateSampler(_deviceObject.GetDevice(), &samplerInfo, nullptr, &sampler));

	_allSamplersStorage.push_back(sampler);

	return sampler;
}

ImageHandle* VulkanImage::LoadAndStoreImageFromFile(const fs::path& path)
{
	stbi_set_flip_vertically_on_load(true);

	int texWidth, texHeight, texChannels;

	stbi_uc* pixels = stbi_load(path.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
	{
		std::cout << "Failed to load image by path: " << path << '\n';
		return nullptr;
	}

	void* pixelsVoid = pixels;

	void* const& data = nullptr;

	const size_t imageSize = texWidth * texHeight * 4; // 4 bytes

	StagingPair stagingPair = _bufferObject.CreateStagingBuffer(pixelsVoid, imageSize);

	stbi_image_free(pixels);


	VkExtent3D imageExtent;
	imageExtent.width = static_cast<u32>(texWidth);
	imageExtent.height = static_cast<u32>(texHeight);
	imageExtent.depth = 1;

	const u32 mipLevels = static_cast<u32>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	VkFormat imgFormat = VK_FORMAT_R8G8B8A8_SRGB; // SURFACE FORMAT IS BGRA BUT WOULD USE THAT, COULD JUST BLIT IMAGE LATER BEFORE PRESENTATION
	// FOR MORE CONVENIENT APPOACH

	VkImageCreateInfo createInfo = vkhelpers::CreateImageInfo(imgFormat, imageExtent, mipLevels, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

	ImageHandle imgHandle;

	VK_CHECK(vmaCreateImage(_allocatorObject.GetAllocatorHandle(), &createInfo, &allocInfo, &imgHandle.image, &imgHandle.allocation, nullptr));

	// TO DO: destroy staging buffer

	VkImageViewCreateInfo imgViewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imgViewCreateInfo.image = imgHandle.image;
	imgViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imgViewCreateInfo.format = imgFormat;
	imgViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imgViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imgViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imgViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imgViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imgViewCreateInfo.subresourceRange.layerCount = 1;
	imgViewCreateInfo.subresourceRange.levelCount = 1;

	VK_CHECK(vkCreateImageView(_deviceObject.GetDevice(), &imgViewCreateInfo, nullptr, &imgHandle.imageView));

	imgHandle.index = _allImagesStorage.size() + 1; // starting from 1, zero is empty texture


	LayoutsUpdateDesc layoutsUpdateDesc;
	layoutsUpdateDesc.stagingPair = stagingPair;
	layoutsUpdateDesc.imgExtent = imageExtent;
	layoutsUpdateDesc.imgHandle = imgHandle;

	_queueToChangeLayouts.push(layoutsUpdateDesc);

	_allImagesStorage.push_back(imgHandle);
	
	return &_allImagesStorage.back();
}

void VulkanImage::UpdateLayoutsToCopyData()
{
	while (!_queueToChangeLayouts.empty())
	{
		const auto& queueFront = _queueToChangeLayouts.front();

		assert(queueFront.stagingPair.buffer && "Trying to change images layout when staging buffer is nullptr");
		// image initially created with undefined layout
		vkhelpers::TransitionImageLayout(_frameObject.GetCommandBuffer(), queueFront.imgHandle.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			0, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);

		VkBufferImageCopy copyRegion{};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;



		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = queueFront.imgExtent;


		vkCmdCopyBufferToImage(_frameObject.GetCommandBuffer(), queueFront.stagingPair.buffer->GetVkBuffer(), queueFront.imgHandle.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		// Transit image again to make it readable for the shader
		vkhelpers::TransitionImageLayout(_frameObject.GetCommandBuffer(), queueFront.imgHandle.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);

		_queueToChangeLayouts.pop();
	}


	// TO DO STAGING BUFFER DELETION
}

void VulkanImage::Cleanup()
{
	for (auto& image : _allImagesStorage)
	{
		vkDestroyImageView(_deviceObject.GetDevice(), image.imageView, nullptr);
		vmaDestroyImage(_allocatorObject.GetAllocatorHandle(), image.image, image.allocation);
	}

	for (auto& sampler : _allSamplersStorage)
	{
		vkDestroySampler(_deviceObject.GetDevice(), sampler, nullptr);
	}
}