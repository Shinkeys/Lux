#include "../../../../headers/base/gfx/raytracing/vk_acceleration_structure.h"
#include "../../../../headers/base/core/raytracing/RT_acceleration_structure.h"
#include "../../../../headers/base/gfx/vk_device.h"
#include "../../../../headers/base/gfx/vk_buffer.h"
#include "../../../../headers/base/gfx/vk_deleter.h"
#include "../../../../headers/base/gfx/vk_command_buffer.h"
#include "../../../../headers/base/gfx/vk_frame.h"
#include "../../../../headers/base/gfx/vk_allocator.h"
#include "../../../../headers/util/gfx/vk_helpers.h"
#include "../../../../headers/asset/asset_types.h"


VulkanAccelerationStructure::VulkanAccelerationStructure(VulkanDevice& deviceObj, VulkanFrame& frameObj,
	VulkanAllocator& allocatorObj, const BLASSpecification& blasSpec) : _deviceObj{deviceObj}
{
	assert(blasSpec.vertexAddress != 0 && blasSpec.indexAddress != 0
		&& "Trying to create vk acceleration structure, but vertex or index address or both are null");


	// Describe buffers as array of Vertex object in this case
	VkAccelerationStructureGeometryTrianglesDataKHR triangles{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR };
	triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	triangles.vertexData.deviceAddress = blasSpec.vertexAddress;
	triangles.vertexStride = blasSpec.vertexStride;
	triangles.maxVertex = blasSpec.verticesCount - 1;
	triangles.transformData.deviceAddress = 0;



	triangles.indexType = VK_INDEX_TYPE_UINT32;
	triangles.indexData.deviceAddress = blasSpec.indexAddress;




	// Identify above data as containing triangles
	VkAccelerationStructureGeometryKHR geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
	geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	geometry.geometry.triangles = triangles;


	const u32 maxPrimitiveCount = blasSpec.indicesCount / 3;

	// build BLAS itself
	VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
	rangeInfo.firstVertex = 0;
	rangeInfo.primitiveOffset = 0;
	rangeInfo.transformOffset = 0;
	rangeInfo.primitiveCount = maxPrimitiveCount;

	VkAccelerationStructureBuildGeometryInfoKHR buildAS{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
	buildAS.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	buildAS.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	buildAS.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	buildAS.geometryCount = 1;
	buildAS.pGeometries = &geometry;


	VkAccelerationStructureBuildSizesInfoKHR buildSizes{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
	vkGetAccelerationStructureBuildSizesKHR(deviceObj.GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&buildAS, &maxPrimitiveCount, &buildSizes);

	// TO DO A WAY TO SHRINK BLAS AS IT IS NOW NOT THAT EFFICIENT IN TERMS OF MEMORY

	// Create acceleration structure itself
	{
		BufferSpecification accelBuffSpec{};
		accelBuffSpec.usage = BufferUsage::ACCELERATION_STRUCTURE_STORAGE | BufferUsage::SHADER_DEVICE_ADDRESS;
		accelBuffSpec.size = buildSizes.accelerationStructureSize;
		accelBuffSpec.memoryUsage = MemoryUsage::AUTO_PREFER_DEVICE;
		accelBuffSpec.memoryProp = MemoryProperty::DEVICE_LOCAL;
		accelBuffSpec.sharingMode = SharingMode::SHARING_EXCLUSIVE;


		_asBuffer = std::make_unique<VulkanBuffer>(accelBuffSpec, deviceObj, allocatorObj, frameObj);

		VulkanBuffer* asBuffRaw = static_cast<VulkanBuffer*>(_asBuffer.get());

		VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
		createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		createInfo.size = buildSizes.accelerationStructureSize;
		createInfo.buffer = asBuffRaw->GetRawBuffer();
		createInfo.offset = 0;

		// Just allocates it basically, now need to fill it
		VK_CHECK(vkCreateAccelerationStructureKHR(deviceObj.GetDevice(), &createInfo, nullptr, &_acceleration));
	}

	// Create buffer
	BufferSpecification spec{};
	spec.usage = BufferUsage::STORAGE_BUFFER | BufferUsage::SHADER_DEVICE_ADDRESS;
	spec.size = buildSizes.buildScratchSize;
	spec.memoryUsage = MemoryUsage::AUTO_PREFER_DEVICE;
	spec.memoryProp = MemoryProperty::DEVICE_LOCAL;
	spec.sharingMode = SharingMode::SHARING_EXCLUSIVE;

	VulkanBuffer scratchBuffer(spec, deviceObj, allocatorObj, frameObj);

	buildAS.dstAccelerationStructure = _acceleration;
	buildAS.scratchData.deviceAddress = scratchBuffer.GetBufferAddress();



	// TO DO: one per submesh, one command BLAS per mesh.
	const VkAccelerationStructureBuildRangeInfoKHR rangeInformations[]
	{
		rangeInfo
	};

	const VkAccelerationStructureBuildRangeInfoKHR* rangeInfosPtrs[]
	{
		rangeInformations
	};

	VulkanCommandBuffer cmdBuffer(deviceObj, frameObj.GetCommandPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	// Yes, separate cmd buffer for this as would be not that great to do a lot of stuff to build BLAS in the main one
	cmdBuffer.BeginRecording();
	vkCmdBuildAccelerationStructuresKHR(cmdBuffer.GetRawBuffer(), 1, &buildAS, rangeInfosPtrs);

	cmdBuffer.EndRecording();
	constexpr bool shouldWait = true;

	cmdBuffer.Submit(shouldWait);

}



VulkanAccelerationStructure::VulkanAccelerationStructure(VulkanDevice& deviceObj, VulkanFrame& frameObj,
	VulkanAllocator& allocatorObj, const TLASSpecification& tlasSpec) : _deviceObj{ deviceObj }
{
	std::vector<VkAccelerationStructureInstanceKHR> tlasInstances;

	for (const auto& blasInstance : tlasSpec.instances)
	{
		VkAccelerationStructureInstanceKHR instance{};

		// transform matrix from glm to vulkan
		// VULKAN MATRICES ARE ROW MAJOR
		VkTransformMatrixKHR matrix{};
		for (u32 row = 0; row < 3; ++row)
		{
			for (u32 col = 0; col < 4; ++col)
			{
				matrix.matrix[row][col] = blasInstance.transform[col][row];
			}
		}

		instance.transform = matrix;
		instance.instanceCustomIndex = blasInstance.customIndex;
		instance.accelerationStructureReference = blasInstance.blasAddress;
		instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR; // disable culling, otherwise would have a perf. penalty

		instance.mask = 0xFF; // hit only if rayMask & instanceMask != 0
		instance.instanceShaderBindingTableRecordOffset = 0; // TO DO

		tlasInstances.push_back(instance);
	}


	const u32 instanceCount = static_cast<u32>(tlasInstances.size());

	// Create buffer
	BufferSpecification instancesBuffSpec{};
	instancesBuffSpec.usage = BufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY | BufferUsage::SHADER_DEVICE_ADDRESS;
	instancesBuffSpec.size = tlasInstances.size() * sizeof(VkAccelerationStructureInstanceKHR);
	instancesBuffSpec.memoryUsage = MemoryUsage::AUTO_PREFER_DEVICE;
	instancesBuffSpec.memoryProp = MemoryProperty::DEVICE_LOCAL;                     // Rework staging dealloc
	instancesBuffSpec.sharingMode = SharingMode::SHARING_EXCLUSIVE;					 // Rework staging dealloc
	instancesBuffSpec.allocCmdBuff = true;

	VulkanBuffer instancesBuffer(instancesBuffSpec, deviceObj, allocatorObj, frameObj);
	instancesBuffer.UploadData(0, tlasInstances.data(), tlasInstances.size() * sizeof(VkAccelerationStructureInstanceKHR));

	VkAccelerationStructureGeometryInstancesDataKHR vkInstances{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
	vkInstances.data.deviceAddress = instancesBuffer.GetBufferAddress();
	vkInstances.arrayOfPointers = VK_FALSE;


	VkAccelerationStructureGeometryKHR topASGeometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
	topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	topASGeometry.geometry.instances = vkInstances;
	topASGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;


	VkAccelerationStructureBuildGeometryInfoKHR buildAS{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
	buildAS.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	buildAS.geometryCount = 1;
	buildAS.pGeometries = &topASGeometry;
	buildAS.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	buildAS.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	buildAS.srcAccelerationStructure = VK_NULL_HANDLE;

	VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
	vkGetAccelerationStructureBuildSizesKHR(deviceObj.GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildAS, &instanceCount, &sizeInfo);

	BufferSpecification accelBuffSpec{};
	accelBuffSpec.usage = BufferUsage::ACCELERATION_STRUCTURE_STORAGE | BufferUsage::SHADER_DEVICE_ADDRESS;
	accelBuffSpec.size = sizeInfo.accelerationStructureSize;
	accelBuffSpec.memoryUsage = MemoryUsage::AUTO_PREFER_DEVICE;
	accelBuffSpec.memoryProp = MemoryProperty::DEVICE_LOCAL;
	accelBuffSpec.sharingMode = SharingMode::SHARING_EXCLUSIVE;

	_asBuffer = std::make_unique<VulkanBuffer>(accelBuffSpec, deviceObj, allocatorObj, frameObj);

	VulkanBuffer* asBuffRaw = static_cast<VulkanBuffer*>(_asBuffer.get());

	// Create accel struct itself
	VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
	createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	createInfo.size = sizeInfo.accelerationStructureSize;
	createInfo.buffer = asBuffRaw->GetRawBuffer();
	createInfo.offset = 0;
	// Just allocates it basically, now need to fill it
	VK_CHECK(vkCreateAccelerationStructureKHR(deviceObj.GetDevice(), &createInfo, nullptr, &_acceleration));

	// Create  scratch buffer
	BufferSpecification scratchBuffSpec{};
	scratchBuffSpec.usage = BufferUsage::STORAGE_BUFFER | BufferUsage::SHADER_DEVICE_ADDRESS;
	scratchBuffSpec.size = sizeInfo.buildScratchSize;
	scratchBuffSpec.memoryProp = MemoryProperty::DEVICE_LOCAL;
	scratchBuffSpec.memoryUsage = MemoryUsage::AUTO_PREFER_DEVICE;
	scratchBuffSpec.sharingMode = SharingMode::SHARING_EXCLUSIVE;

	VulkanBuffer scratchBuffer(scratchBuffSpec, deviceObj, allocatorObj, frameObj);

	buildAS.srcAccelerationStructure = VK_NULL_HANDLE;
	buildAS.dstAccelerationStructure = _acceleration;
	buildAS.scratchData.deviceAddress = scratchBuffer.GetBufferAddress();


	//// Build Offsets info: n instances
	VkAccelerationStructureBuildRangeInfoKHR        buildOffsetInfo;
	buildOffsetInfo.firstVertex = 0;
	buildOffsetInfo.primitiveCount = instanceCount;
	buildOffsetInfo.primitiveOffset = 0;
	buildOffsetInfo.transformOffset = 0;

	const VkAccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo[] = { &buildOffsetInfo };

	VulkanCommandBuffer cmdBuffer(deviceObj, frameObj.GetCommandPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	cmdBuffer.BeginRecording();

	vkCmdBuildAccelerationStructuresKHR(cmdBuffer.GetRawBuffer(), 1, &buildAS, pBuildOffsetInfo);

	cmdBuffer.EndRecording();
	constexpr bool shouldWait = true;
	cmdBuffer.Submit(shouldWait);

}


u64 VulkanAccelerationStructure::GetAccelerationAddress() const
{
	VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
	addressInfo.accelerationStructure = _acceleration;
	return vkGetAccelerationStructureDeviceAddressKHR(_deviceObj.GetDevice(), &addressInfo);
}

VulkanAccelerationStructure::~VulkanAccelerationStructure()
{
	VkDevice device = _deviceObj.GetDevice();
	VkAccelerationStructureKHR accel = _acceleration;

	VulkanDeleter::SubmitObjectDesctruction([device, accel]() {
		vkDestroyAccelerationStructureKHR(device, accel, nullptr);
		});
}