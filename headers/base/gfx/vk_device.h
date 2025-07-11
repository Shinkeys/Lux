#pragma once
#include "vk_instance.h"


enum class QueueType : u8
{
	VULKAN_GRAPHICS_QUEUE = 0,
	VULKAN_PRESENTATION_QUEUE = 1,
	// To do other types
	
	VULKAN_QUEUE_COUNT = 2,
};

class VulkanDevice
{
private:
	VulkanInstance& _instanceObject;

	VkPhysicalDevice _physDevice{ VK_NULL_HANDLE };
	VkDevice _device{ VK_NULL_HANDLE };

	float _maxAnisotropy{ 0.0f };

	// Physical device
	VkPhysicalDevice SelectAppropriatePhysDevice(std::vector<VkPhysicalDevice>& physDevices) const;
	void CreatePhysicalDevice();
	VkBool32 FamilySupportsPresentation(VkPhysicalDevice physDevice, u32 familyIndex) const;
	VkBool32 QueryExtensionsSupport(VkPhysicalDevice physDevice) const;
	std::optional<u32> GetGraphicsFamilyIndex(VkPhysicalDevice physDevice, VkQueueFlagBits flagBits) const;
	const std::vector<const char*> _requiredDeviceExtensions
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	std::optional<u32> GetPresentationFamilyIndex() const;

	bool QueryPhysDeviceFeatures(VkPhysicalDevice physDevice) const;
	void QueryAnisotropyLevel();

	// Logical device
	void CreateLogicalDevice();
	
	// Queues
	// They're separate because might need only queue indices someday
	std::array<std::optional<u32>, static_cast<size_t>(QueueType::VULKAN_QUEUE_COUNT)> _queueFamIndexStorage;
	std::array<std::optional<VkQueue>, static_cast<size_t>(QueueType::VULKAN_QUEUE_COUNT)> _queuesStorage;
public:
	float GetMaxAnisotropyLevel() const { return _maxAnisotropy; }

	std::vector<u32> GetGraphicsFamilyIndices(VkPhysicalDevice physDevice, VkQueueFlagBits flagBits) const;
	VkPhysicalDevice GetPhysicalDevice() const { return _physDevice; }
	VkDevice GetDevice() const { return _device; }
	std::optional<VkQueue> GetQueueByType(QueueType type)
	{
		if (_queuesStorage[static_cast<size_t>(type)] == std::nullopt)
			return std::nullopt;

		return _queuesStorage[static_cast<size_t>(type)];
	}
	std::optional<u32> GetQueueIndexByType(QueueType type)
	{
		if (_queueFamIndexStorage[static_cast<size_t>(type)] == std::nullopt)
			return std::nullopt;

		return _queueFamIndexStorage[static_cast<size_t>(type)];
	}


	VulkanDevice() = delete;
	~VulkanDevice() = default;
	VulkanDevice(VulkanInstance& instanceObj);
	

	VulkanDevice(const VulkanDevice&) = delete;
	VulkanDevice(VulkanDevice&&) = delete;
	VulkanDevice& operator= (const VulkanDevice&) = delete;
	VulkanDevice& operator= (VulkanDevice&&) = delete;

	void Cleanup();
};

