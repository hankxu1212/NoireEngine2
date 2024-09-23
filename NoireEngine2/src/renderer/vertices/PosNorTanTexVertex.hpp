#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include "backend/commands/CommandBuffer.hpp"

struct PosNorTanTexVertex
{
	struct { float x, y, z; } Position;
	struct { float x, y, z; } Normal;
	struct { float x, y, z, b; } Tangent;
	struct { float s, t; } TexCoord;

	static const VkPipelineVertexInputStateCreateInfo array_input_state;

	static void Bind(const CommandBuffer& commandBuffer);
};

static_assert(sizeof(PosNorTanTexVertex) == 3 * 4 + 3 * 4 + 4 * 4 + 2 * 4, "PosNorTexVertex is packed.");
