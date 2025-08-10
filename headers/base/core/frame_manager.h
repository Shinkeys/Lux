#pragma once
#include "../../util/util.h"

class VulkanBase;
class FrameManager
{
private:
	VulkanBase& _vulkanBase;

public:
	FrameManager(VulkanBase& vulkanBase);

	u32 GetCurrentImageIndex() const;
	u32 GetCurrentFrameIndex() const;

};