#pragma once
#include "../../../util/util.h"

class RTAccelerationStructure;
class RTPipeline;
class VulkanBase;
class ShaderBindingTable;
class BufferManager;
struct BLASSpecification;
struct TLASSpecification;
struct SBTSpecification;
struct RTDeviceProperties;

// Purpose: class to handle all the logic related to ray tracing: create objects like TLAS, BLAS etc
class RayTracingManager
{
private:
	VulkanBase& _vulkanBase;
	
	BufferManager& _bufferManager;
public:

	RayTracingManager(VulkanBase& vulkanBase, BufferManager& buffManager);

	std::unique_ptr<RTAccelerationStructure>	 CreateBLAS(const BLASSpecification& spec)	 const;
	std::unique_ptr<RTAccelerationStructure>	 CreateTLAS(const TLASSpecification& spec)	 const;
	std::unique_ptr<ShaderBindingTable>          CreateSBT(const  SBTSpecification& spec)    const;

	void GetRTGroupHandles(RTPipeline* rtPipeline, u32 firstGroup, u32 groupCount, u32 dataSize, void* outData)  const;

	RTDeviceProperties GetRTDeviceProperties() const;
};