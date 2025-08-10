#include "../../../headers/base/core/presentation_manager.h"
#include "../../../headers/base/gfx/vk_presentation.h"
#include "../../../headers/base/gfx/vk_base.h"
#include "../../../headers/base/core/image.h"

PresentationManager::PresentationManager(VulkanBase& vulkanBase) : _vulkanBase{vulkanBase}
{

}


ImageExtent2D  PresentationManager::GetSwapchainExtent() const
{
	return vkconversions::ToEngineExtent2D(_vulkanBase.GetPresentationObj().GetSwapchainDesc().extent);
}

Image* PresentationManager::GetSwapchainImage(u32 imageIndex) const
{
	return _vulkanBase.GetPresentationObj().GetSwapchainDesc().images.at(imageIndex).get();
}