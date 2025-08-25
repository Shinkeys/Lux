#include "../../../headers/base/gfx/vk_base.h"
#include "../../../headers/base/gfx/vk_deleter.h"
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
	_shaderObject = std::make_unique<VulkanShader>(*_deviceObject);
}


VulkanBase::~VulkanBase()
{
	vkDeviceWaitIdle(_deviceObject->GetDevice());

	_frameObject->Cleanup();
	_presentationObject->Cleanup();
	_allocatorObject->Cleanup();
	_deviceObject->Cleanup();
	_instanceObject->Cleanup();

	VulkanDeleter::ExecuteDeletion(true);

}