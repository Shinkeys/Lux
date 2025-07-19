#pragma once
#include "../util/util.h"

class Buffer;
struct DeviceIndirectBuffer
{
	std::unique_ptr<Buffer> opaqueBuffer{ nullptr };
	std::unique_ptr<Buffer> maskBuffer{ nullptr };


	std::unique_ptr<Buffer> commonDataBuffer{ nullptr };

	std::unique_ptr<Buffer> commonIndicesBuffer{ nullptr };

	size_t currentOpaqueSize{ 0 };
	size_t currentMaskedSize{ 0 };
	size_t currentCommonDataOffset{ 0 };
	size_t currentCommonIndicesOffset{ 0 };

	size_t countBufferOffset{ 0 };
};