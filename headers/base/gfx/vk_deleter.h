#pragma once
#include "../../util/gfx/vk_types.h"
#include "../../util/util.h"

// Static class which holds the queue to delete objects
class VulkanDeleter
{
private:
	using Func = std::function<void()>;
	using FramesSinceSubmission = u32;

	static std::vector<std::pair<FramesSinceSubmission, Func>> _deletionQueue;
public:
	static void SubmitObjectDesctruction(Func&& function);
	static void ExecuteDeletion(bool deviceIdle = false);
};