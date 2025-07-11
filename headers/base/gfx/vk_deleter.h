#pragma once
#include "../../util/gfx/vk_types.h"
#include "../../util/util.h"

// Static class which holds the queue to delete objects
class VulkanDeleter
{
private:
	using Func = std::function<void()>;

	static std::queue<Func> _deletionQueue;
public:
	static void SubmitObjectDesctruction(Func&& function);
	static void ExecuteDeletion();
};