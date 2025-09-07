#pragma once
#include "../util/util.h"
#include "../base/core/raytracing/RT_acceleration_structure.h"

struct BLASContainer
{
	std::unique_ptr<RTAccelerationStructure> accel{ nullptr };
	BLASInstance instance{};
};