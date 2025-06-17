#include "../../headers/util/logger.h"

#include <vulkan/vk_enum_string_helper.h>


const char* Logger::ToString(VkResult result)	 	{ return string_VkResult(result); }

const char* Logger::ToString(VkPresentModeKHR mode)	{ return string_VkPresentModeKHR(mode); }

std::string Logger::ToString(VkExtent2D extent) 
{
	std::ostringstream oss;
	oss << "Width: " << extent.width << " Height: " << extent.height;
	return oss.str();
}

const char* Logger::ToString(bool value) { if (value) return "true"; return "false"; }

const char* Logger::ToString(LogLevel level)
{
	switch (level)
	{
	case LogLevel::Debug:
		return "[DEBUG]";

	case LogLevel::Log:
		return "[LOG]";

	case LogLevel::Warn:
		return "[WARN]";

	case LogLevel::Fatal:
		return "[FATAL]";

	default: 
		return "?????";
	}
}