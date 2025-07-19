#include "../../../headers/base/gfx/vk_deleter.h"

std::vector<std::pair<VulkanDeleter::FramesSinceSubmission, 
	VulkanDeleter::Func>> VulkanDeleter::_deletionQueue;


void VulkanDeleter::SubmitObjectDesctruction(Func&& function)
{
	// zero frames since submission
	_deletionQueue.push_back({ 0, function });
}

void VulkanDeleter::ExecuteDeletion(bool deviceIdle)
{
	// Uses vector because it's more convenient, tho would need reallocations
	// after deletion, this queue is not intented to be large, so this is not a problem
	for(auto it = _deletionQueue.begin(); it != _deletionQueue.end();)
	{
		if (deviceIdle || it->first >= 3) // passed 3 frames since execution
		{
			it->second();
			it = _deletionQueue.erase(it);
		}
		else
		{
			++it->first;

			++it;
		}
	}
}