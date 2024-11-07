#include "PBRMaterialPipeline.hpp"

#include "renderer/materials/PBRMaterial.hpp"
#include "backend/shader/VulkanShader.h"
#include "backend/VulkanContext.hpp"
#include "backend/pipeline/VulkanGraphicsPipelineBuilder.hpp"

void PBRMaterialPipeline::Create()
{
	CreatePipelineLayout();
	CreateGraphicsPipeline();
}

void PBRMaterialPipeline::CreatePipelineLayout()
{
	std::array< VkDescriptorSetLayout, 5 > layouts{
		Renderer::Instance->set0_WorldLayout,
		Renderer::Instance->set1_StorageBuffersLayout,
		Renderer::Instance->set2_TexturesLayout,
		Renderer::Instance->set3_CubemapLayout,
		Renderer::Instance->set4_ShadowMapLayout,
	};

	VkPushConstantRange range{
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0,
		.size = sizeof(PBRMaterial::MaterialPush),
	};

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = uint32_t(layouts.size()),
		.pSetLayouts = layouts.data(),
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &range,
	};

	VulkanContext::VK(vkCreatePipelineLayout(VulkanContext::GetDevice(), &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout),
		"[Vulkan] Create pipeline layout failed.");
}

void PBRMaterialPipeline::CreateGraphicsPipeline()
{
	std::vector< VkDynamicState > dynamic_states{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_VERTEX_INPUT_EXT
	};

	std::array< VkPipelineColorBlendAttachmentState, 1 > attachment_states{
		VkPipelineColorBlendAttachmentState{
			.blendEnable = VK_FALSE,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		},
	};

	VulkanGraphicsPipelineBuilder::Start()
		.SetDynamicStates(dynamic_states)
		.SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.SetColorBlending((uint32_t)attachment_states.size(), attachment_states.data())
		.Build("../spv/shaders/pbr.vert.spv", "../spv/shaders/pbr.frag.spv", &m_Pipeline, m_PipelineLayout, Renderer::Instance->m_Renderpass->renderpass);
}
