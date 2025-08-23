#pragma once
#include "../../util/util.h"

enum class BufferUsage : u32
{
	NONE = 0,
	TRANSFER_SRC = 0x00000001,
	TRANSFER_DST = 0x00000002,
	UNIFORM_BUFFER  = 0x00000004,
	STORAGE_BUFFER  = 0x00000008,
	INDEX_BUFFER    = 0x00000016,
	VERTEX_BUFFER   = 0x00000032,
	INDIRECT_BUFFER = 0x00000064,
	SHADER_DEVICE_ADDRESS = 0x00000128,
	ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY = 0x00000256,
	ACCELERATION_STRUCTURE_STORAGE = 0x00000512,
	SHADER_BINDING_TABLE = 0x00001024,
};

inline bool operator&(BufferUsage fst, BufferUsage scd)
{
	return (static_cast<u32>(fst) & static_cast<u32>(scd)) != 0;
}

inline BufferUsage operator|(BufferUsage fst, BufferUsage scd)
{
	return static_cast<BufferUsage>(static_cast<u32>(fst) | static_cast<u32>(scd));
}


enum class SharingMode : u8
{
	SHARING_EXCLUSIVE = 0,
	SHARING_CONCURRENT = 1,
};

enum class MemoryUsage : u8
{
	UNKNOWN,
	AUTO,
	AUTO_PREFER_DEVICE,
	AUTO_PREFER_HOST,
};


enum class MemoryProperty : u32
{
	NONE = 0,
	DEVICE_LOCAL = 0x00000001,
	HOST_VISIBLE = 0x00000002,
	HOST_COHERENT = 0x00000004,
	HOST_CACHED = 0x00000008
};

inline bool operator&(MemoryProperty fst, MemoryProperty scd)
{
	return (static_cast<u32>(fst) & static_cast<u32>(scd)) != 0;
}


inline MemoryProperty operator|(MemoryProperty fst, MemoryProperty scd)
{
	return static_cast<MemoryProperty>(static_cast<u32>(fst) | static_cast<u32>(scd));
}

enum class AllocationCreate : u32
{
	NONE = 0,
	DEDICATED_MEMORY = 0x00000001,
	NEVER_ALLOCATE = 0x00000002,
	MAPPED = 0x00000004,
	WITHIN_BUDGET = 0x00000040,
	HOST_ACCESS_SEQUENTIAL_WRITE = 0x00000100,
	HOST_ACCESS_RANDOM = 0x00000200,
	STRATEGY_BEST_FIT = 0x00010000,
	STRATEGY_FIRST_FIT = 0x00040000
};

inline bool operator&(AllocationCreate fst, AllocationCreate scd)
{
	return (static_cast<u32>(fst) & static_cast<u32>(scd)) != 0;
}