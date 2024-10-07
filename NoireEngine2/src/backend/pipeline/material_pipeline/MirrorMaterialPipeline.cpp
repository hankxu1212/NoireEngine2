#include "MirrorMaterialPipeline.hpp"

#include "backend/shader/VulkanShader.h"
#include "backend/VulkanContext.hpp"
#include "renderer/vertices/PosColVertex.hpp"
#include "renderer/components/CameraComponent.hpp"
#include "renderer/scene/Scene.hpp"
#include "core/resources/Files.hpp"

static uint32_t vert_code[] =
#include "spv/shaders/mirror.vert.inl"
;

static uint32_t frag_code[] =
#include "spv/shaders/mirror.frag.inl"
;

MirrorMaterialPipeline::MirrorMaterialPipeline(ObjectPipeline* objectPipeline) :
	p_ObjectPipeline(objectPipeline)
{
	cube = ImageCube::Create(Files::Path("../scenes/SkyboxClouds"), ".png");
}

MirrorMaterialPipeline::~MirrorMaterialPipeline()
{
	m_DescriptorAllocator.Cleanup(); // destroy pool and sets
	m_DescriptorLayoutCache.Cleanup(); // destroy all set layouts
}

void MirrorMaterialPipeline::Create()
{
	CreateDescriptors();
	CreatePipelineLayout();
	CreateGraphicsPipeline();
}

void MirrorMaterialPipeline::BindDescriptors(const CommandBuffer& commandBuffer, uint32_t surfaceId)
{
	ObjectPipeline::Workspace& workspace = p_ObjectPipeline->workspaces[surfaceId];
	std::array< VkDescriptorSet, 4 > descriptor_sets{
		workspace.set0_World,
		workspace.set1_Transforms,
		p_ObjectPipeline->set2_Textures,
		set3_Cubemap
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

void MirrorMaterialPipeline::CreateGraphicsPipeline()
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

	VkGraphicsPipelineCreateInfo create_info{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = uint32_t(stages.size()),
		.pStages = stages.data(),
		.pVertexInputState = VK_NULL_HANDLE,
		.pInputAssemblyState = &input_assembly_state,
		.pViewportState = &viewport_state,
		.pRasterizationState = &rasterization_state,
		.pMultisampleState = &multisample_state,
		.pDepthStencilState = &depth_stencil_state,
		.pColorBlendState = &color_blend_state,
		.pDynamicState = &dynamic_state,
		.layout = m_PipelineLayout,
		.renderPass = p_ObjectPipeline->m_Renderpass->renderpass,
		.subpass = 0,
	};

	VulkanContext::VK_CHECK(
		vkCreateGraphicsPipelines(VulkanContext::GetDevice(), VulkanContext::Get()->getPipelineCache(), 1, &create_info, nullptr, &m_Pipeline),
		"[Vulkan] Create pipeline failed");
}

void MirrorMaterialPipeline::CreatePipelineLayout()
{
	std::array< VkDescriptorSetLayout, 4 > layouts{
		p_ObjectPipeline->set0_WorldLayout,
		p_ObjectPipeline->set1_TransformsLayout,
		p_ObjectPipeline->set2_TexturesLayout,
		set3_CubemapLayout
	};

	VkPipelineLayoutCreateInfo create_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = uint32_t(layouts.size()),
		.pSetLayouts = layouts.data(),
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr,
	};

	VulkanContext::VK_CHECK(vkCreatePipelineLayout(VulkanContext::GetDevice(), &create_info, nullptr, &m_PipelineLayout));
}

void MirrorMaterialPipeline::CreateDescriptors()
{
	VkDescriptorImageInfo cubeMapInfo
	{
		.sampler = cube->getSampler(),
		.imageView = cube->getView(),
		.imageLayout = cube->getLayout()
	};

	DescriptorBuilder::Start(&m_DescriptorLayoutCache, &m_DescriptorAllocator)
		.BindImage(0, &cubeMapInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.Build(set3_Cubemap, set3_CubemapLayout);
}