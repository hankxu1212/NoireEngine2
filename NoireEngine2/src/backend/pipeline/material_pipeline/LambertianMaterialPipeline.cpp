#include "LambertianMaterialPipeline.hpp"

#include "renderer/materials/LambertianMaterial.hpp"
#include "backend/shader/VulkanShader.h"
#include "backend/VulkanContext.hpp"
#include "backend/pipeline/VulkanGraphicsPipelineBuilder.hpp"

LambertianMaterialPipeline::LambertianMaterialPipeline(ObjectPipeline* objectPipeline) :
	MaterialPipeline(objectPipeline)
{
}

void LambertianMaterialPipeline::Create()
{
	CreatePipelineLayout();
	CreateGraphicsPipeline();
}

void LambertianMaterialPipeline::BindDescriptors(const CommandBuffer& commandBuffer)
{
	ObjectPipeline::Workspace& workspace = p_ObjectPipeline->workspaces[CURR_FRAME];
	std::array< VkDescriptorSet, 4 > descriptor_sets{
		workspace.set0_World,
		workspace.set1_Transforms,
		p_ObjectPipeline->set2_Textures,
		p_ObjectPipeline->set3_Cubemap
	};
	vkCmdBindDescriptorSets(
		commandBuffer, //command buffer
		VK_PIPELINE_BIND_POINT_GRAPHICS, //pipeline bind point
		m_PipelineLayout, //pipeline layout
		0, //first set
		uint32_t(descriptor_sets.size()), descriptor_sets.data(), //descriptor sets count, ptr
		0, nullptr //dynamic offsets count, ptr
	);
}

void LambertianMaterialPipeline::CreatePipelineLayout()
{
	std::array< VkDescriptorSetLayout, 4 > layouts{
		p_ObjectPipeline->set0_WorldLayout,
		p_ObjectPipeline->set1_TransformsLayout,
		p_ObjectPipeline->set2_TexturesLayout,
		p_ObjectPipeline->set3_CubemapLayout,
	};

	VkPushConstantRange range{
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0,
		.size = sizeof(LambertianMaterial::MaterialPush),
	};

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = uint32_t(layouts.size()),
		.pSetLayouts = layouts.data(),
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &range,
	};

	VulkanContext::VK_CHECK(vkCreatePipelineLayout(VulkanContext::GetDevice(), &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout),
		"[Vulkan] Create pipeline layout failed.");
}

void LambertianMaterialPipeline::CreateGraphicsPipeline()
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
		.Build("../spv/shaders/lambertian.vert.spv", "../spv/shaders/lambertian.frag.spv", &m_Pipeline, m_PipelineLayout, p_ObjectPipeline->m_Renderpass->renderpass);
}
