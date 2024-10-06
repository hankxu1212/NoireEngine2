#pragma once

#include <vulkan/vulkan_core.h>
#include <cstdint>

#include "math/color/Color.hpp"

struct PosVertex 
{
	glm::vec3 Position;
	static const VkPipelineVertexInputStateCreateInfo array_input_state;
};

static_assert(sizeof(PosVertex) == 12, "PosColVertex is packed.");