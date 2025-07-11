#include "../../../headers/base/gfx/vk_instance.h"
#include "../../../headers/base/gfx/vk_deleter.h"
#include "../../../headers/base/window.h"
#include "../../../headers/util/logger.h"


#include <SDL3/SDL_vulkan.h>

VulkanInstance::VulkanInstance(Window& window) : _windowObject{ window }
{
	CreateInstance();
#ifdef KHRONOS_VALIDATION
	CreateDebugMessenger();
#endif
	CreateSurface();
}


bool VulkanInstance::IsLayerSupported(const char* name)
{
	u32 propertyCount = 0;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&propertyCount, 0));

	std::vector<VkLayerProperties> properties(propertyCount);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&propertyCount, properties.data()));

	for (u32 i = 0; i < propertyCount; ++i)
	{
		if (strcmp(name, properties[i].layerName) == 0)
			return true;
	}

	return false;
}

bool VulkanInstance::IsInstanceExtensionSupported(const char* name)
{
	u32 propertyCount = 0;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, 0));

	std::vector<VkExtensionProperties> properties(propertyCount);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, properties.data()));

	for (u32 i = 0; i < propertyCount; ++i)
	{
		if (strcmp(name, properties[i].extensionName) == 0)
			return true;
	}

	return false;
}


void VulkanInstance::CreateInstance()
{
	VK_CHECK(volkInitialize());

	if (volkGetInstanceVersion() < API_VERSION)
	{
		Logger::CriticalLog("Vulkan error : Vulkan 1." + std::to_string(VK_API_VERSION_MINOR(API_VERSION)) + " instance not found");
	}

	VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.apiVersion = API_VERSION;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = "Lux";
	appInfo.pEngineName = "VULKAN";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

	VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appInfo;
	createInfo.pNext = nullptr;


#ifdef KHRONOS_VALIDATION
	// Additional usage of validation layers to enable them for instance
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

	std::vector<const char*> debugLayers
	{
		"VK_LAYER_KHRONOS_validation"
	};

	if (IsLayerSupported("VK_LAYER_KHRONOS_validation"))
	{
		createInfo.ppEnabledLayerNames = debugLayers.data();
		createInfo.enabledLayerCount = static_cast<u32>(debugLayers.size());
		FillDebugMessenger(debugCreateInfo);
		createInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
		Logger::Log("[DEBUG] Enabled validation layers", true, LogLevel::Debug);
	}
	else
	{
		std::cout << "Vulkan error: Vulkan validation layers are not available\n";
	}
#endif

	std::vector<const char*> extensions;
	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef VK_USE_PLATFORM_WIN32_KHR
	extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
	extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
	extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif

	if (IsInstanceExtensionSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.enabledExtensionCount = static_cast<u32>(extensions.size());


	VK_CHECK(vkCreateInstance(&createInfo, nullptr, &_instance));

	volkLoadInstanceOnly(_instance);
}


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{

	std::cerr << "[VALIDATION LAYER] " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void VulkanInstance::FillDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	// Maybe turn off performance messages, to see.
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr;
}

void VulkanInstance::CreateDebugMessenger()
{
	// Maybe turn off performance messages, to see.
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	FillDebugMessenger(createInfo);

	auto funcP = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT"));
	if (funcP == nullptr) throw std::runtime_error("[FATAL] failed to load vkCreateDebugUtilsMessengerEXT");

	funcP(_instance, &createInfo, nullptr, &_debugMessenger);
}

void VulkanInstance::CreateSurface()
{
	bool isSurfaceCreated = SDL_Vulkan_CreateSurface(_windowObject.GetWindowPtr(), _instance, nullptr, &_surface);
	Logger::Log("Vulkan surface created", true, isSurfaceCreated, LogLevel::Debug);
}

void VulkanInstance::Cleanup()
{
	VulkanDeleter::SubmitObjectDesctruction([this]() {
#ifdef KHRONOS_VALIDATION
		auto funcP = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT"));
		if (funcP == nullptr) throw std::runtime_error("[FATAL] failed to load failed to load vkDestroyDebugUtilsMessengerEXT");

		funcP(_instance, _debugMessenger, nullptr);
#endif

		vkDestroySurfaceKHR(_instance, _surface, nullptr);
		vkDestroyInstance(_instance, nullptr);
		});
}