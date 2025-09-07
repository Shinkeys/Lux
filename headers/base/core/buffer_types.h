#pragma once
#include "../../util/util.h"

enum class BufferUsage : u32
{
	NONE = 0,
	TRANSFER_SRC = 1 << 0,
	TRANSFER_DST = 1 << 1,
	UNIFORM_BUFFER  = 1 << 2,
	STORAGE_BUFFER  = 1 << 3,
	INDEX_BUFFER    = 1 << 4,
	VERTEX_BUFFER   = 1 << 5,
	INDIRECT_BUFFER = 1 << 6,
	SHADER_DEVICE_ADDRESS = 1 << 7,
	ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY = 1 << 8,
	ACCELERATION_STRUCTURE_STORAGE = 1 << 9,
	SHADER_BINDING_TABLE = 1 << 10,
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
	UNKNOWN = 0,
	AUTO = 1,
	AUTO_PREFER_DEVICE = 2,
	AUTO_PREFER_HOST = 3,
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
	DEDICATED_MEMORY = 1 << 0,
	NEVER_ALLOCATE = 1 << 1,
	MAPPED = 1 << 2,
	WITHIN_BUDGET = 1 << 3,
	HOST_ACCESS_SEQUENTIAL_WRITE = 1 << 4,
	HOST_ACCESS_RANDOM = 1 << 5,
	STRATEGY_BEST_FIT = 1 << 6,
	STRATEGY_FIRST_FIT = 1 << 7
};


inline bool operator&(AllocationCreate fst, AllocationCreate scd)
{
	return (static_cast<u32>(fst) & static_cast<u32>(scd)) != 0;
}