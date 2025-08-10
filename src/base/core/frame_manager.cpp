#include "../../../headers/base/core/frame_manager.h"
#include "../../../headers/base/gfx/vk_base.h"

FrameManager::FrameManager(VulkanBase& vulkanBase) : _vulkanBase{vulkanBase}
{

}


u32 FrameManager::GetCurrentImageIndex() const
{
	return _vulkanBase.GetFrameObj().GetCurrentImageIndex();
}

u32 FrameManager::GetCurrentFrameIndex() const
{
	return _vulkanBase.GetFrameObj().GetCurrentFrameIndex();
}