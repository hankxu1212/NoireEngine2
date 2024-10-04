#include "LambertianMaterialPipeline.hpp"

#include <array>

#include "renderer/materials/LambertianMaterial.hpp"
#include "backend/shader/VulkanShader.h"
#include "backend/VulkanContext.hpp"

static uint32_t vert_code[] =
#include "spv/shaders/objects.vert.inl"
;

static uint32_t frag_code[] =
#include "spv/shaders/objects.frag.inl"
;

LambertianMaterialPipeline::LambertianMaterialPipeline(ObjectPipeline* objectPipeline) :
	MaterialPipeline(objectPipeline)
{
}

void LambertianMaterialPipeline::Create()
{
	VulkanShader vertModule(vert_code, VulkanShader::ShaderStage::Vertex);
	VulkanShader fragModule(frag_code, VulkanShader::ShaderStage::Frag);

	std::array< VkPipelineShaderStageCreateInfo, 2 > stages{
		vertModule.shaderStage(),
		fragModule.shaderStage()
	};

	///////////////////////////////////////////////////////////////////////

	std::vector< VkDynamicState > dynamic_states{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_VERTEX_INPUT_EXT
	};

	VkPipelineDynamicStateCreateInfo dynamic_state{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = uint32_t(dynamic_states.size()),
		.pDynamicStates = dynamic_states.data()
	};


	//this pipeline will take no per-vertex inputs:
	VkPipelineVertexInputStateCreateInfo vertex_input_state{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = nullptr,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = nullptr,
	};

	//this pipeline will draw triangles:
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	//this pipeline will render to one viewport and scissor rectangle:
	VkPipelineViewportStateCreateInfo viewport_state{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,
	};

	//the rasterizer will cull back faces and fill polygons:
	VkPipelineRasterizationStateCreateInfo rasterization_state{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.0f,
	};

	//multisampling will be disabled (one sample per pixel):
	VkPipelineMultisampleStateCreateInfo multisample_state{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
	};

	//depth test will be less, and stencil test will be disabled:
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
	};

	//there will be one color attachment with blending disabled:
	std::array< VkPipelineColorBlendAttachmentState, 1 > attachment_states{
		VkPipelineColorBlendAttachmentState{
			.blendEnable = VK_FALSE,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		},
	};
	VkPipelineColorBlendStateCreateInfo color_blend_state{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.attachmentCount = uint32_t(attachment_states.size()),
		.pAttachments = attachment_states.data(),
		.blendConstants{0.0f, 0.0f, 0.0f, 0.0f},
	};

	//all of the above structures get bundled together into one very large create_info:
	VkGraphicsPipelineCreateInfo create_info{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = uint32_t(stages.size()),
		.pStages = stages.data(),
		.pVertexInputState = /*&VertexInput::array_input_state*/VK_NULL_HANDLE,
		.pInputAssemblyState = &input_assembly_state,
		.pViewportState = &viewport_state,
		.pRasterizationState = &rasterization_state,
		.pMultisampleState = &multisample_state,
		.pDepthStencilState = &depth_stencil_state,
		.pColorBlendState = &color_blend_state,
		.pDynamicState = &dynamic_state,
		.layout = m_PipelineLayout,
		.renderPass = p_ObjectPipeline->m_Renderpass,
		.subpass = 0,
	};

	VulkanContext::VK_CHECK(
		vkCreateGraphicsPipelines(VulkanContext::GetDevice(), VK_NULL_HANDLE, 1, &create_info, nullptr, &m_Pipeline),
		"[Vulkan] Create pipeline failed");
}

void LambertianMaterialPipeline::BindDescriptors(const CommandBuffer& commandBuffer, Material* materialInstance)
{
	//LambertianMaterial* lambertianMaterialInstance = dynamic_cast<LambertianMaterial*>(materialInstance);
	ObjectPipeline::Workspace& workspace = p_ObjectPipeline->workspaces[0]; // TODO: pass in surface id
	{ //bind Transforms descriptor set:
		std::array< VkDescriptorSet, 2 > descriptor_sets{
			workspace.World_descriptors, //0: World
			workspace.Transforms_descriptors, //1: Transforms
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

	// this is binded per-material pipeline
	//bind texture descriptor set: (temporary, dont look)
	vkCmdBindDescriptorSets(
		commandBuffer, //command buffer
		VK_PIPELINE_BIND_POINT_GRAPHICS, //pipeline bind point
		m_PipelineLayout, //pipeline layout
		2, //second set
		1, &p_ObjectPipeline->G_GLOBAL_TEXTURE_SET[0], //descriptor sets count, ptr
		0, nullptr //dynamic offsets count, ptr
	);
}
