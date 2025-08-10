#pragma once
#include "../../util/gfx/vk_types.h"

enum class CommandBufferType : u8
{
	TYPE_GRAPHICS_SUBMIT,
	TYPE_COMPUTE_SUBMIT,
};


class VulkanDevice;
class VulkanCommandBuffer
{
private:
	VulkanDevice& _deviceObject;

	VkCommandBuffer _cmdBuffer;
	VkCommandPool _cmdPool;

public:
	~VulkanCommandBuffer();
	VulkanCommandBuffer(VulkanDevice& deviceObject, VkCommandPool cmdPool, VkCommandBufferLevel level);

	VkCommandBuffer GetRawBuffer() const { return _cmdBuffer; }

	// one time submit by default
	void BeginRecording(VkCommandBufferUsageFlags usage = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	void Submit(bool waitBeforeExecution = false);
	void EndRecording();

};