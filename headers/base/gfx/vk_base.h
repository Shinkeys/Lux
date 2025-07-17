#pragma once
#include "vk_buffer.h"
#include "vk_descriptor.h"
#include "vk_allocator.h"
#include "vk_image.h"
#include "vk_shader.h"

// Purpose: class-holder of the various vulkan structures. Would implement basic initialization.
// This is the only place where volk would be included, as well as the only place
// where VOLK_IMPLEMENTATION would be defined
class VulkanBase
{
private:
	std::unique_ptr<VulkanInstance> _instanceObject;
	std::unique_ptr<VulkanDevice> _deviceObject;
	std::unique_ptr<VulkanPresentation> _presentationObject;
	std::unique_ptr<VulkanFrame> _frameObject;
	std::unique_ptr<VulkanBuffer> _bufferObject;
	std::unique_ptr<VulkanAllocator> _allocatorObject;
	std::unique_ptr<VulkanShader> _shaderObject;

public:
	VulkanInstance& GetVulkanInstanceObj()		{ return *_instanceObject; }
	VulkanDevice& GetVulkanDeviceObj()		    { return *_deviceObject; }
	VulkanPresentation& GetPresentationObj()    { return *_presentationObject; }
	VulkanFrame& GetFrameObj()					{ return *_frameObject; }
	VulkanBuffer& GetBufferObj()				{ return *_bufferObject; }
	VulkanAllocator& GetAllocatorObj()			{ return *_allocatorObject; }
	VulkanShader& GetShaderObj()				{ return *_shaderObject; }


	VulkanBase() = default;
	~VulkanBase();
	VulkanBase(const VulkanBase&) = delete;
	VulkanBase& operator=(const VulkanBase&) = delete;
	VulkanBase(VulkanBase&&) noexcept = delete;
	VulkanBase& operator=(VulkanBase&&) noexcept = delete;

	void Initialize(Window& windowObj);
};