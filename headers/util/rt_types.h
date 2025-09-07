#pragma once

struct RTDeviceProperties
{
	u32 shaderGroupHandleSize{ 0 };
	u32 shaderGroupBaseAlignment{ 0 };
	u32 shaderGroupHandleAlignment{ 0 };



	// accel struct
	u32 minAccelScratchOffsetAlignment{ 0 };
};


struct SBTSpecification
{
	u32 missCount{ 0 };
	u32 hitCount{ 0 };

	std::vector<byte> handles;
};
