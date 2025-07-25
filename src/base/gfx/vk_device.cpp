#include "../../../headers/base/gfx/vk_device.h"
#include "../../../headers/util/gfx/vk_defines.h"
#include "../../../headers/base/gfx/vk_deleter.h"

VulkanDevice::VulkanDevice(VulkanInstance& instanceObj) : _instanceObject{instanceObj}
{
	CreatePhysicalDevice();
	CreateLogicalDevice();
}

void VulkanDevice::Cleanup()
{
	VulkanDeleter::SubmitObjectDesctruction([this]() {
		vkDestroyDevice(_device, nullptr);
	});
}

// To do: for different queue families, like:
// (physDevice, desiredQueueFam) -> return true if found
std::optional<u32> VulkanDevice::GetGraphicsFamilyIndex(VkPhysicalDevice physDevice, VkQueueFlagBits flagBits) const
{
	u32 qCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &qCount, 0);

	std::vector<VkQueueFamilyProperties> queues(qCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &qCount, queues.data());

	for (u32 i = 0; i < qCount; ++i)
	{
		if (queues[i].queueFlags & flagBits)
			return i;
	}

	return std::nullopt;
}

std::vector<u32> VulkanDevice::GetGraphicsFamilyIndices(VkPhysicalDevice physDevice, VkQueueFlagBits flagBits) const
{
	u32 qCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &qCount, 0);

	std::vector<VkQueueFamilyProperties> queues(qCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &qCount, queues.data());

	std::vector<u32> resultQueues;
	for (u32 i = 0; i < qCount; ++i)
	{
		if (queues[i].queueFlags & flagBits)
			resultQueues.push_back(i);
	}

	return resultQueues;
}

std::optional<u32> VulkanDevice::GetPresentationFamilyIndex() const
{
	auto familyIndices = GetGraphicsFamilyIndices(_physDevice, VK_QUEUE_GRAPHICS_BIT);

	for (u32 famIndex : familyIndices)
	{
		VkBool32 presentSupported = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(_physDevice, famIndex, _instanceObject.GetSurface(), &presentSupported);

		if (presentSupported)
			return famIndex;
	}

	return std::nullopt;
}


VkBool32 VulkanDevice::FamilySupportsPresentation(VkPhysicalDevice physDevice, u32 familyIndex) const
{
#ifdef VK_USE_PLATFORM_WIN32_KHR
	return vkGetPhysicalDeviceWin32PresentationSupportKHR(physDevice, familyIndex);
#else
	return true;
#endif
}

bool VulkanDevice::QueryPhysDeviceFeatures(VkPhysicalDevice physDevice) const
{
	VkPhysicalDeviceVulkan12Features queryVulkan12Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	VkPhysicalDeviceVulkan13Features queryVulkan13Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };

	queryVulkan12Features.pNext = &queryVulkan13Features;

	VkPhysicalDeviceFeatures2 features2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
	features2.pNext = &queryVulkan12Features;


	vkGetPhysicalDeviceFeatures2(physDevice, &features2);
	// Write required features here
	if (!queryVulkan13Features.dynamicRendering)
		return false;
	if (!queryVulkan13Features.synchronization2)
		return false;

	if (!queryVulkan12Features.bufferDeviceAddress)
		// bufferDeviceAddressCaptureReplay for debug support if would need
		return false;
	if (!queryVulkan12Features.scalarBlockLayout)
		return false;

	if (!queryVulkan12Features.descriptorIndexing ||
		!queryVulkan12Features.shaderSampledImageArrayNonUniformIndexing ||
		!queryVulkan12Features.descriptorBindingSampledImageUpdateAfterBind ||
		!queryVulkan12Features.descriptorBindingPartiallyBound ||
		!queryVulkan12Features.descriptorBindingStorageImageUpdateAfterBind ||
		!queryVulkan12Features.runtimeDescriptorArray)
		return false;

	return true;
}


// Purpose: method to write all the needed phys. device extensions
// Return value: true if found all the extension, false otherwise, meaning that
// device is not suitable
VkBool32 VulkanDevice::QueryExtensionsSupport(VkPhysicalDevice physDevice) const
{
	u32 extensionsCount;
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionsCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, & extensionsCount, availableExtensions.data());

	std::unordered_set<std::string_view> requiredExtensionsSet(_requiredDeviceExtensions.begin(), _requiredDeviceExtensions.end());

	u32 foundExtensions = 0;
	for (const auto& extension : availableExtensions)
	{
		if (requiredExtensionsSet.find(extension.extensionName) != requiredExtensionsSet.end())
			++foundExtensions;
	}

	return foundExtensions == static_cast<u32>(_requiredDeviceExtensions.size());
}

VkPhysicalDevice VulkanDevice::SelectAppropriatePhysDevice(std::vector<VkPhysicalDevice>& physDevices) const
{
	VkPhysicalDevice preferred = 0;
	VkPhysicalDevice fallback  = 0;

	// To rework if would need better logic to select appropriate GPU, but it works well anyway in case of 2- GPUs
	for (const auto& device : physDevices)
	{
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(device, &props);

		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
			continue;


		std::cout << "Available phys. devices: " << props.deviceName << " Vulkan version: 1." 
			<< VK_VERSION_MINOR(props.apiVersion) << std::endl;

		auto familyIndex = GetGraphicsFamilyIndex(device, VK_QUEUE_GRAPHICS_BIT);
		// No graphics queue family on this device
		if (!familyIndex.has_value())
			continue;

		if(!QueryExtensionsSupport(device))
			continue;

		if (!QueryPhysDeviceFeatures(device))
			continue;

		if (!preferred && props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			preferred = device;

		if (!fallback)
			fallback = device;
	}

	VkPhysicalDevice result = preferred ? preferred : fallback;
	// no physical devices
	Logger::Log("Vulkan phys. device selection failed", result, LogLevel::Fatal);

	if (result)
	{
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(result, &props);
		std::cout << "Selected GPU: " << props.deviceName << '\n';
	}
	else
	{
		std::cout << "Vulkan error: no compatible GPU found\n";
	}


	return result;
}


void VulkanDevice::CreatePhysicalDevice()
{
	u32 physDeviceCount = 0;
	vkEnumeratePhysicalDevices(_instanceObject.GetInstance(), &physDeviceCount, nullptr);

	std::vector<VkPhysicalDevice> physDevices(physDeviceCount);
	vkEnumeratePhysicalDevices(_instanceObject.GetInstance(), &physDeviceCount, physDevices.data());
	
	_physDevice = SelectAppropriatePhysDevice(physDevices);
}

void VulkanDevice::QueryAnisotropyLevel()
{
	VkPhysicalDeviceProperties props{};
	vkGetPhysicalDeviceProperties(_physDevice, &props);

	_maxAnisotropy = props.limits.maxSamplerAnisotropy;
}

void VulkanDevice::CreateLogicalDevice()
{
	auto graphicsFamIndex = GetGraphicsFamilyIndex(_physDevice, VK_QUEUE_GRAPHICS_BIT);
	Logger::Log("Vulkan found graphics queue family", true, graphicsFamIndex.has_value(), LogLevel::Fatal);

	auto presentationFamIndex = GetPresentationFamilyIndex();
	Logger::Log("Vulkan found presentation queue family", true, presentationFamIndex.has_value(), LogLevel::Debug);

	// Vulkan spec states: queue family indices should be unique only, so unordered_set
	std::unordered_set<u32> qFamilies
	{
		graphicsFamIndex.value(),
		presentationFamIndex.value()
	};
	std::vector<VkDeviceQueueCreateInfo> qCreateInfos;

	float queuesPriority[]{ 1.0f };
	for (u32 qFamily : qFamilies)
	{
		VkDeviceQueueCreateInfo qCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		qCreateInfo.queueFamilyIndex = qFamily;
		qCreateInfo.queueCount = 1;
		qCreateInfo.pQueuePriorities = queuesPriority; // schedule of cmd buffer execution, must be set

		qCreateInfos.push_back(qCreateInfo);
	}

	VkPhysicalDeviceVulkan12Features vulkan12Features =
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = nullptr,
		.drawIndirectCount = VK_TRUE,
		.descriptorIndexing = VK_TRUE, // BINDLESS
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
		.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE,
		.descriptorBindingPartiallyBound = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE,
	
		.scalarBlockLayout = VK_TRUE,
		.bufferDeviceAddress = VK_TRUE,
	};

	VkPhysicalDeviceVulkan13Features vulkan13Features = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = &vulkan12Features,
		.synchronization2 = VK_TRUE,
		.dynamicRendering = VK_TRUE,
	};

	VkPhysicalDeviceVulkan11Features vulkan11Features =
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
		.pNext = &vulkan13Features,
		.shaderDrawParameters = VK_TRUE
	};

	VkPhysicalDeviceFeatures2 deviceFeatures2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
	deviceFeatures2.features.fragmentStoresAndAtomics = VK_TRUE; // to remove or create slang issue on git
	deviceFeatures2.pNext = &vulkan11Features;

	VkDeviceCreateInfo deviceCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.pNext = &deviceFeatures2;
	deviceCreateInfo.pQueueCreateInfos = qCreateInfos.data();
	deviceCreateInfo.queueCreateInfoCount = static_cast<u32>(qCreateInfos.size());

	deviceCreateInfo.ppEnabledExtensionNames = _requiredDeviceExtensions.data();
	deviceCreateInfo.enabledExtensionCount = static_cast<u32>(_requiredDeviceExtensions.size());

	VK_CHECK(vkCreateDevice(_physDevice, &deviceCreateInfo, nullptr, &_device));

	volkLoadDevice(_device);

	QueryAnisotropyLevel();


	// Adding all the queues
	const u32 graphicsQInd = graphicsFamIndex.value();
	const u32 presentationQInd = presentationFamIndex.value();

	// Add graphics q to the storage
	_queueFamIndexStorage[static_cast<size_t>(QueueType::VULKAN_GRAPHICS_QUEUE)] = graphicsQInd;
	VkQueue graphicsQueue;
	vkGetDeviceQueue(_device, graphicsQInd, 0, &graphicsQueue);
	_queuesStorage[static_cast<size_t>(QueueType::VULKAN_GRAPHICS_QUEUE)] = graphicsQueue;

	// Add presentation q to the storage
	_queueFamIndexStorage[static_cast<size_t>(QueueType::VULKAN_PRESENTATION_QUEUE)] = presentationQInd;
	VkQueue presentationQueue;
	vkGetDeviceQueue(_device, presentationQInd, 0, &presentationQueue);
	_queuesStorage[static_cast<size_t>(QueueType::VULKAN_PRESENTATION_QUEUE)] = presentationQueue;
}