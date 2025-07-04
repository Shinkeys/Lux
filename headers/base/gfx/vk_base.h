#pragma once
#include "vk_buffer.h"
#include "vk_descriptor.h"
#include "vk_allocator.h"
#include "vk_image.h"

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
	std::unique_ptr<VulkanDescriptor> _descriptorObject;
	std::unique_ptr<VulkanAllocator> _allocatorObject;
	std::unique_ptr<VulkanImage> _imageObject;

public:
	VulkanInstance& GetVulkanInstanceObj()		{ return *_instanceObject; }
	VulkanDevice& GetVulkanDeviceObj()		    { return *_deviceObject; }
	VulkanPresentation& GetPresentationObj()    { return *_presentationObject; }
	VulkanPipeline& GetPipelineObj()            { return *_pipelineObject; }
	VulkanFrame& GetFrameObj()					{ return *_frameObject; }
	VulkanBuffer& GetBufferObj()				{ return *_bufferObject; }
	VulkanDescriptor& GetDescriptorObj()		{ return *_descriptorObject; }
	VulkanAllocator& GetAllocatorObj()			{ return *_allocatorObject; }
	VulkanImage& GetImageObj()					{ return *_imageObject; }

	void Cleanup();
	void Initialize(Window& windowObj);
	void Update();
};