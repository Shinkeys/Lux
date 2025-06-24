#pragma once
#include "vk_buffer.h"

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
	std::unique_ptr<VulkanBuffer> _bufferObject;

public:
	VulkanInstance& GetVulkanInstanceObj()		{ return *_instanceObject; }
	VulkanDevice& GetVulkanDeviceObj()		    { return *_deviceObject; }
	VulkanPresentation& GetPresentationObj()    { return *_presentationObject; }
	VulkanPipeline& GetPipelineObj()            { return *_pipelineObject; }
	VulkanFrame& GetFrameObj()					{ return *_frameObject; }
	VulkanBuffer& GetBufferObj()				{ return *_bufferObject; }

	void RenderFrame();
	void Cleanup();
	void Initialize(Window& windowObj);
};