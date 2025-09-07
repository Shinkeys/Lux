#include "../../../headers/base/core/image.h"
#include "../../../headers/base/gfx/vk_image.h"
#include "../../../headers/base/gfx/vk_base.h"

std::unique_ptr<Image> ImageManager::CreateImage(const ImageSpecification& spec)	   const
{
	return std::make_unique<VulkanImage>(spec, _vulkanBase.GetVulkanDeviceObj(), _vulkanBase.GetFrameObj(), _vulkanBase.GetAllocatorObj());
}

std::unique_ptr<Sampler> ImageManager::CreateSampler(const SamplerSpecification& spec) const
{
	return std::make_unique<VulkanSampler>(spec, _vulkanBase.GetVulkanDeviceObj());
}

ImageManager::ImageManager(VulkanBase& vulkanBase) : _vulkanBase{vulkanBase}
{

}

