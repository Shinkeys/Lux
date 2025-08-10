#pragma once
#include "../../../util/util.h"

class RTAccelerationStructure;
class VulkanBase;
struct BLASSpecification;
struct TLASSpecification;

// Purpose: class to handle all the logic related to raytracing: create objects like tlases, blases etc
class RayTracingManager
{
private:
	VulkanBase& _vulkanBase;
	
public:

	RayTracingManager(VulkanBase& vulkanBase);

	std::unique_ptr<RTAccelerationStructure>	 CreateBLAS(const BLASSpecification& spec)	 const;
	std::unique_ptr<RTAccelerationStructure>	 CreateTLAS(const TLASSpecification& spec)	 const;

};