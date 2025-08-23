#pragma once
#include "../../../util/util.h"


struct SBTRegion
{
	u64 address{ 0 };
	u64 size{ 0 };
	u64 stride{ 0 };
};

struct RTDeviceProperties;
struct SBTSpecification;
class BufferManager;
class Buffer;

class ShaderBindingTable
{
private:
	SBTRegion _raygenTable;
	SBTRegion _missTable;
	SBTRegion _closestTable;

	std::unique_ptr<Buffer> _buffer;
public:

	ShaderBindingTable(const RTDeviceProperties& rtProperties, const SBTSpecification& createInfo, const BufferManager& buffManager);

	SBTRegion GetRaygenTable() const { return _raygenTable; }
	SBTRegion GetMissTable() const   { return _missTable; }
	SBTRegion GetClosestTable() const { return _closestTable; }

	
};