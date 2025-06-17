#pragma once
#include "vk_frame.h"

// Purpose: class-holder of the various vulkan structures. Would implement basic initialization.
// This is the only place where volk would be included, as well as the only place
// where VOLK_IMPLEMENTATION would be defined
class VulkanBase
{
private:
	std::unique_ptr<VulkanInstance> _instanceObject;
	std::unique_ptr<VulkanDevice> _deviceObject;
	std::unique_ptr<VulkanPresentation> _presentationObject;
	std::unique_ptr<VulkanPipeline> _pipelineObject;
	std::unique_ptr<VulkanFrame> _frameObject;
public:
	VulkanInstance* GetVulkanInstanceObj() { return _instanceObject.get(); }

	void RenderFrame();
	void Cleanup();
	void Initialize(Window& windowObj);
};