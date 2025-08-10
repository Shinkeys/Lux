#include "../../../../headers/base/gfx/vk_command_buffer.h"
#include "../../../../headers/base/gfx/vk_deleter.h"
#include "../../../../headers/base/gfx/vk_device.h"
#include "../../../../headers/util/gfx/vk_helpers.h"

VulkanCommandBuffer::~VulkanCommandBuffer()
{
	assert(_cmdPool != VK_NULL_HANDLE && "Trying to free command buffer with null cmd pool");
	VkCommandBuffer cmdBuffer = _cmdBuffer;
	VkCommandPool cmdPool = _cmdPool;
	VkDevice device = _deviceObject.GetDevice();
	VulkanDeleter::SubmitObjectDesctruction([device, cmdPool, cmdBuffer]()
		{
			vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuffer);
		});
}

VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice& deviceObject, VkCommandPool cmdPool, VkCommandBufferLevel level)
	: _deviceObject{ deviceObject }, _cmdPool { cmdPool }
{
	VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandPool = _cmdPool;
	allocateInfo.commandBufferCount = 1;
	allocateInfo.level = level;

	VK_CHECK(vkAllocateCommandBuffers(_deviceObject.GetDevice(), &allocateInfo, &_cmdBuffer));
}

void VulkanCommandBuffer::BeginRecording(VkCommandBufferUsageFlags usage /* = one time submit*/)
{
	VkCommandBufferBeginInfo beginInfo = vkhelpers::CmdBufferBeginInfo(usage);

	VK_CHECK(vkBeginCommandBuffer(_cmdBuffer, &beginInfo));
}

void VulkanCommandBuffer::Submit(bool waitBeforeExecution)
{
	// FOR NOW GENERAL
	std::optional<VkQueue> generalQueue = _deviceObject.GetQueueByType(QueueType::VULKAN_GENERAL_QUEUE);
	assert(generalQueue.has_value() && "Trying to get general queue which is empty, cannot submit cmd buffer");
	
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_cmdBuffer;
	
	VkFence fence = VK_NULL_HANDLE;

	if (waitBeforeExecution)
	{
		VkFenceCreateInfo createInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		vkCreateFence(_deviceObject.GetDevice(), &createInfo, nullptr, &fence);
	}

	VK_CHECK(vkQueueSubmit(generalQueue.value(), 1, &submitInfo, fence));

	if (waitBeforeExecution)
	{
		VkDevice device = _deviceObject.GetDevice();
		vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
		vkDestroyFence(device, fence, nullptr);
	}

}

void VulkanCommandBuffer::EndRecording()
{
	VK_CHECK(vkEndCommandBuffer(_cmdBuffer));
}