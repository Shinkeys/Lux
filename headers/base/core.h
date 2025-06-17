#pragma once
#include <iostream>

#include "window.h"
#include "../util/util.h"


// Purpose: handles engine state updates, render loop. Basically everything in-game related.
// except API creation. Would contain all the needed structures. Would be handled separately from the API
// but with the possibility to call Vulkan-specific functions if needed.
class Core
{
private:

public:
	bool Initialize();
	void Render();
	void Update();
	void Cleanup();


};