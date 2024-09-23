#include "PosNorTanTexVertex.hpp"

#include <array>
#include "backend/VulkanContext.hpp"

static std::array< VkVertexInputBindingDescription, 1 > bindings{
	VkVertexInputBindingDescription{
		.binding = 0,
		.stride = sizeof(PosNorTanTexVertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	}
};

static std::array< VkVertexInputAttributeDescription, 4 > attributes{
	VkVertexInputAttributeDescription{
		.location = 0,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(PosNorTanTexVertex, Position),
	},
	VkVertexInputAttributeDescription{
		.location = 1,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(PosNorTanTexVertex, Normal),
	},
	VkVertexInputAttributeDescription{
		.location = 2,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32A32_SFLOAT,
		.offset = offsetof(PosNorTanTexVertex, Tangent),
	},
	VkVertexInputAttributeDescription{
		.location = 3,
		.binding = 0,
		.format = VK_FORMAT_R32G32_SFLOAT,
		.offset = offsetof(PosNorTanTexVertex, TexCoord),
	},
};

const VkPipelineVertexInputStateCreateInfo PosNorTanTexVertex::array_input_state{
	.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	.vertexBindingDescriptionCount = uint32_t(bindings.size()),
	.pVertexBindingDescriptions = bindings.data(),
	.vertexAttributeDescriptionCount = uint32_t(attributes.size()),
	.pVertexAttributeDescriptions = attributes.data(),
};

void PosNorTanTexVertex::Bind(const CommandBuffer& commandBuffer)
{
	static std::array<VkVertexInputBindingDescription2EXT, 1>  vertex_bindings_description_ext{
		VkVertexInputBindingDescription2EXT
		{
			VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT,
			nullptr,
			0,
			sizeof(PosNorTanTexVertex),
			VK_VERTEX_INPUT_RATE_VERTEX,
			1
		}
	};
	
	static std::array<VkVertexInputAttributeDescription2EXT, 4> vertex_attribute_description_ext = {
		VkVertexInputAttributeDescription2EXT{
			.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(PosNorTanTexVertex, Position),
		},
		VkVertexInputAttributeDescription2EXT{
			.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(PosNorTanTexVertex, Normal),
		},
		VkVertexInputAttributeDescription2EXT{
			.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
			.location = 2,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32A32_SFLOAT,
			.offset = offsetof(PosNorTanTexVertex, Tangent),
		},
		VkVertexInputAttributeDescription2EXT{
			.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
			.location = 3,
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(PosNorTanTexVertex, TexCoord),
		},
	};

	/* First set of vertex input dynamic data (Vertex structure) */
	//vertex_bindings_description_ext[0].stride = sizeof(Vertex);
	//vertex_attribute_description_ext[1].offset = offsetof(Vertex, normal);

	auto func = (PFN_vkCmdSetVertexInputEXT)vkGetInstanceProcAddr(*VulkanContext::Get()->getInstance(), "vkCmdSetVertexInputEXT");
	func(commandBuffer,
		static_cast<uint32_t>(vertex_bindings_description_ext.size()),
		vertex_bindings_description_ext.data(),
		static_cast<uint32_t>(vertex_attribute_description_ext.size()),
		vertex_attribute_description_ext.data());
}
