#include "../../../headers/base/core/raytracing/shader_binding_table.h"
#include "../../../headers/util/rt_types.h"
#include "../../../headers/base/core/buffer.h"

inline u32 AlignUp(u32 value, u32 alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

ShaderBindingTable::ShaderBindingTable(const RTDeviceProperties& rtProperties, const SBTSpecification& createInfo, const BufferManager& buffManager)
{
	assert(createInfo.hitCount > 0 && createInfo.missCount > 0 && "Hit count or miss count or both are null, cannot create shader binding table");
	assert(rtProperties.shaderGroupHandleSize > 0 && "Shader group handle size is 0 from rt device properties");
	assert(!createInfo.handles.empty() && "RT group handles are empty, cannot create shader binding table");

	const u32 handleSizeAligned = AlignUp(rtProperties.shaderGroupHandleSize, rtProperties.shaderGroupHandleAlignment);

	_raygenTable.stride = AlignUp(handleSizeAligned, rtProperties.shaderGroupBaseAlignment); // only one raygen ALWAYS
	_raygenTable.size = _raygenTable.stride;

	_missTable.stride = handleSizeAligned;
	_missTable.size = AlignUp(createInfo.missCount * handleSizeAligned, rtProperties.shaderGroupBaseAlignment);

	_closestTable.stride = handleSizeAligned;
	_closestTable.size = AlignUp(createInfo.hitCount * handleSizeAligned, rtProperties.shaderGroupBaseAlignment);

	const u32 totalHandleCount = 1 + createInfo.missCount + createInfo.hitCount; // 1 for raygen

	const u32 dataSize = totalHandleCount * rtProperties.shaderGroupHandleSize;
	
	const u64 sbtSize = _raygenTable.size + _missTable.size + _closestTable.size;

	{
		BufferSpecification spec{};
		spec.size = sbtSize;
		spec.usage = BufferUsage::TRANSFER_SRC | BufferUsage::SHADER_DEVICE_ADDRESS | BufferUsage::SHADER_BINDING_TABLE;
		spec.memoryProp = MemoryProperty::HOST_VISIBLE | MemoryProperty::HOST_COHERENT;
		spec.memoryUsage = MemoryUsage::AUTO;
		spec.sharingMode = SharingMode::SHARING_EXCLUSIVE;
		spec.allocCreate = AllocationCreate::HOST_ACCESS_SEQUENTIAL_WRITE;


		_buffer = buffManager.CreateBuffer(spec);
	}


	const u64 bufferAddress = _buffer->GetBufferAddress();
	_raygenTable.address = bufferAddress;
	_missTable.address = bufferAddress + _raygenTable.size;
	_closestTable.address = bufferAddress + _raygenTable.size + _missTable.size;

	const u32 handleSize = rtProperties.shaderGroupHandleSize;
	// Helper function to retrieve data by index
	auto getHandle = [&](int i) { return createInfo.handles.data() + i * handleSize; };

	u32 handleIdx = 0;

	// Raygen
	_buffer->UploadData(0, getHandle(handleIdx++), handleSize);

	// Miss
	u32 missOffset = _raygenTable.size;
	for (u32 i = 0; i < createInfo.missCount; ++i)
	{
		_buffer->UploadData(missOffset, getHandle(handleIdx++), handleSize);
		missOffset += _missTable.stride;
	}

	// Closest hit
	/*u32 hitOffset = _raygenTable.size + _missTable.size;*/
	u32 hitOffset = missOffset;
	for (u32 i = 0; i < createInfo.hitCount; ++i)
	{
		_buffer->UploadData(hitOffset, getHandle(handleIdx++), handleSize);
		hitOffset += _closestTable.stride;
	}


}