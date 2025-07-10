#include "../../../headers/base/gfx/vk_base.h"
#include "../../../headers/base/core/renderer.h"

// DEFINE FOR VOLK SHOULD BE PLACED ONLY HERE
#define VOLK_IMPLEMENTATION
#include <volk.h>


void VulkanBase::Initialize(Window& windowObj)
{
	_instanceObject = std::make_unique<VulkanInstance>(windowObj);
	_deviceObject = std::make_unique<VulkanDevice>(*_instanceObject);
	_presentationObject = std::make_unique<VulkanPresentation>(*_instanceObject, *_deviceObject, windowObj);
	_frameObject = std::make_unique<VulkanFrame>(*_deviceObject, *_presentationObject);
	_allocatorObject = std::make_unique<VulkanAllocator>(*_instanceObject, *_deviceObject);
	_bufferObject = std::make_unique<VulkanBuffer>(*_instanceObject, *_deviceObject, *_allocatorObject);
}


void VulkanBase::Cleanup()
{
	vkDeviceWaitIdle(_deviceObject->GetDevice());
	// Instance should be destroyed last
	_bufferObject->Cleanup();
	_allocatorObject->Cleanup();
	_frameObject->Cleanup();
	_presentationObject->Cleanup();
	_deviceObject->Cleanup();
	_instanceObject->Cleanup();
}