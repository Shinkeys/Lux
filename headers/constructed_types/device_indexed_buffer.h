#pragma once
#include "../util/util.h"


class Buffer;
struct DeviceIndexedBuffer
{
	std::unique_ptr<Buffer> vertexBuffer{ nullptr };
	std::unique_ptr<Buffer> indexBuffer{ nullptr };


	size_t currentVertexOffset{ 0 };
	size_t currentIndexOffset { 0 };
};