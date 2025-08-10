#pragma once
#include "../../util/util.h"
#include "image_types.h"

class VulkanBase;
class Image;
class PresentationManager
{
private:
	VulkanBase& _vulkanBase;

public:
	PresentationManager(VulkanBase& vulkanBase);

	ImageExtent2D          GetSwapchainExtent() const;
	Image*                 GetSwapchainImage(u32 imageIndex) const;

};