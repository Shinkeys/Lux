#pragma once
#include "../../../util/util.h"

#include <glm/glm.hpp>

struct BLASSpecification
{
	u64 vertexAddress{ 0 }; // base address to the common buffer of scene vertices
	u64 indexAddress{ 0 };
	u32 verticesCount{ 0 };
	u32 indicesCount{ 0 };
	u32 vertexStride{ 0 };

	bool isOpaque{ false };
};

struct BLASInstance
{
	u32 instanceSBTOffset{ 0 };
	glm::mat4 transform{glm::mat4(1.0f)};
	u32 customIndex{ 0 };
	u64 blasAddress{ 0 };
};

struct TLASSpecification
{
	std::deque<BLASInstance> instances;
};


class RTAccelerationStructure
{
private:


public:
	virtual u64 GetAccelerationAddress() const = 0;

	virtual ~RTAccelerationStructure() { }

};
