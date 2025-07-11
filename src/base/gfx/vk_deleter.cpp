#include "../../../headers/base/gfx/vk_deleter.h"

std::queue<VulkanDeleter::Func> VulkanDeleter::_deletionQueue;

void VulkanDeleter::SubmitObjectDesctruction(Func&& function)
{	
	_deletionQueue.emplace(function);
}

void VulkanDeleter::ExecuteDeletion()
{
	while (!_deletionQueue.empty())
	{
		(_deletionQueue.front())();

		_deletionQueue.pop();
	}
}