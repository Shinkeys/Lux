#pragma once
#include "gfx/vk_types.h"
#include "gfx/vk_defines.h"

enum class LogLevel : u8
{
	Debug = 0,
	Log = 1, // Don't need now
	Warn = 2,
	Fatal = 3
};

class Logger
{
private:
	// If not partially specialized type then false
	template<typename T>
	static constexpr bool IsNullHandle(T value);

	static const char* ToString(LogLevel level);

	static const char* ToString(VkResult result);
	static const char* ToString(VkPresentModeKHR mode);
	static std::string ToString(VkExtent2D extent);
	static const char* ToString(bool value);

public:
	template<typename T>
	static void Log(const std::string& message, T expected, T actual, LogLevel level)
	{
		switch (level)
		{
		case LogLevel::Debug:
		{
#ifdef KHRONOS_VALIDATION
			if (expected != actual)
			{
				std::cout << "Expected result is: " << ToString(expected) << " but actual is: " << ToString(actual) << '\n';
				assert(false);
			}
			std::cout << "[Debug] " << message << " --- " << "complete" << '\n';
#endif
			break;
		}
		case LogLevel::Log:
			//
			break;

		case LogLevel::Warn:
		{
			if (expected != actual)
			{
				std::cerr << "[Warn] " << message << '\n' << "Expected result is: " << ToString(expected) << " but actual is: " << ToString(actual) << '\n';
			}
			break;
		}

		case LogLevel::Fatal:
		{
			if (expected != actual)
			{
				std::cerr << "[FATAL] " << message << '\n' << "Expected result is: " << ToString(expected) << " but actual is: " << ToString(actual) << '\n';
				std::abort();
			}
			break;
		}
		}
	}

	template<typename T>
	static void Log(const std::string& message, T object, LogLevel level)
	{
		bool objectIsNull = IsNullHandle(object);

		Log(message, false, objectIsNull, level);
	}

	template<>
	static void Log<bool>(const std::string& message, bool state, LogLevel level)
	{
		Log(message, true, state, level);
	}

	template<typename T>
	static void Log(const std::string& message, T object)
	{
#ifdef KHRONOS_VALIDATION
		std::cout <<"[Info] " << message << " --- " << ToString(object) << '\n';
#endif
	}


	static void CriticalLog(const std::string& message)
	{
		std::cout << "[CRITICAL] " << message << '\n';
		std::abort();
	}
};


template<typename T>
static constexpr bool Logger::IsNullHandle(T value) { return false; }


template<>
static constexpr bool Logger::IsNullHandle<VkInstance>(VkInstance instance) { return instance == VK_NULL_HANDLE; }
template<>
static constexpr bool Logger::IsNullHandle<VkPhysicalDevice>(VkPhysicalDevice physDevice) { return physDevice == VK_NULL_HANDLE; }
template<>
static constexpr bool Logger::IsNullHandle<VkSwapchainKHR>(VkSwapchainKHR swapchain) { return swapchain == VK_NULL_HANDLE; }