#pragma once
#include "util.h"

namespace helpers
{
	// Purpose: check if in projects root now
	inline bool IsProjectRoot(const fs::path& path)
	{
		return fs::exists(path / "headers") && fs::exists(path / "src");
	}

}