#pragma once
#include "../util/util.h"

class Buffer;
struct DeviceIndirectBuffer
{
	std::unique_ptr<Buffer> opaqueBuffer{ nullptr };
	std::unique_ptr<Buffer> maskBuffer{ nullptr };

	// they're separated because it would be a WAY more convenient to manage, otherwise you would need a separate buffer to manage indices so this is the same basically
	std::unique_ptr<Buffer> commonOpaqueData{ nullptr };
	std::unique_ptr<Buffer> commonMaskedData{ nullptr };


	size_t currentOpaqueSize{ 0 };
	size_t currentMaskedSize{ 0 };
	size_t currentCommonOpaqueDataOffset{ 0 };
	size_t currentCommonMaskedDataOffset{ 0 };

	size_t countBufferOffset{ 0 };
};