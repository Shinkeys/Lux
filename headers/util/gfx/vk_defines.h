#pragma once
#include "../util.h"

#include <volk.h>

// Macro's
#define VK_CHECK(call) \
	do \
	{ \
		VkResult result = call; \
		assert(result == VK_SUCCESS); \
	} while (0)


#define API_VERSION VK_API_VERSION_1_3

#ifndef NDEBUG
#define KHRONOS_VALIDATION
#endif
