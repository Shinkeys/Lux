#include "../../../headers/util/gfx/vk_helpers.h"


namespace vkhelpers
{
	std::optional<VkShaderModule> ReadShaderFile(const fs::path& shaderPath, VkDevice device)
	{
		fs::path completePath = "resources\\shaders\\" / shaderPath;
		completePath = fs::absolute(completePath);

		std::ifstream fileInput;
		fileInput.open(completePath, std::ios::ate | std::ios::binary);

		if (!fileInput.is_open())
		{
			Logger::CriticalLog("Can't load shader file by path: " + completePath.string());
		}

		size_t fileSize = static_cast<size_t>(fileInput.tellg());
		// File size is the size till the end. This buffer basically should be char
		// but then would need to use reinterpret_cast, so this approach is aight.
		std::vector<u32> buffer(fileSize / sizeof(u32));

		// Cursor at the beginning
		fileInput.seekg(0);
		fileInput.read((char*)buffer.data(), fileSize);

		fileInput.close();

		VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		// Should be in BYTES
		createInfo.codeSize = buffer.size() * static_cast<size_t>(sizeof(u32));
		createInfo.pCode = buffer.data();

		VkShaderModule shaderModule;
		VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));

		return shaderModule;
	}

	VkCommandBufferBeginInfo CmdBufferBeginInfo(VkCommandBufferUsageFlags flags)
	{
		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.pNext = nullptr;
		beginInfo.flags = flags;
		beginInfo.pInheritanceInfo = nullptr;

		return beginInfo;
	}

	void TransitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout,
			VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 srcStageMask, VkPipelineStageFlags2 dstStageMask)
	{
		VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
		barrier.oldLayout = currentLayout;
		barrier.newLayout = newLayout;
		barrier.srcAccessMask = srcAccessMask;
		barrier.dstAccessMask = dstAccessMask;
		barrier.srcStageMask = srcStageMask;
		barrier.dstStageMask = dstStageMask;

		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = image;

		barrier.subresourceRange =
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		};

		VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
		dependencyInfo.dependencyFlags = 0;
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &barrier;

		vkCmdPipelineBarrier2(cmd, &dependencyInfo);
	}
}