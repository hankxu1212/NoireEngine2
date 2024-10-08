#include "EnvironmentMaterialPipeline.hpp"

#include "backend/shader/VulkanShader.h"
#include "backend/VulkanContext.hpp"
#include "renderer/vertices/PosColVertex.hpp"
#include "renderer/components/CameraComponent.hpp"
#include "renderer/scene/Scene.hpp"
#include "core/resources/Files.hpp"
#include "backend/pipeline/VulkanGraphicsPipelineBuilder.hpp"

EnvironmentMaterialPipeline::EnvironmentMaterialPipeline(ObjectPipeline* objectPipeline) :
	p_ObjectPipeline(objectPipeline)
{
	cube = ImageCube::Create(Files::Path("../scenes/SkyboxClouds"), ".png");
}

EnvironmentMaterialPipeline::~EnvironmentMaterialPipeline()
{
	m_DescriptorAllocator.Cleanup(); // destroy pool and sets
}

void EnvironmentMaterialPipeline::Create()
{
	CreateDescriptors();
	CreatePipelineLayout();
	CreateGraphicsPipeline();
}

void EnvironmentMaterialPipeline::BindDescriptors(const CommandBuffer& commandBuffer)
{
	ObjectPipeline::Workspace& workspace = p_ObjectPipeline->workspaces[CURR_FRAME];
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

void EnvironmentMaterialPipeline::CreateGraphicsPipeline()
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
		.Build("../spv/shaders/environment.vert.spv", "../spv/shaders/environment.frag.spv", &m_Pipeline, m_PipelineLayout, p_ObjectPipeline->m_Renderpass->renderpass);
}

void EnvironmentMaterialPipeline::CreatePipelineLayout()
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

void EnvironmentMaterialPipeline::CreateDescriptors()
{
	VkDescriptorImageInfo cubeMapInfo
	{
		.sampler = cube->getSampler(),
		.imageView = cube->getView(),
		.imageLayout = cube->getLayout()
	};

	DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
		.BindImage(0, &cubeMapInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.Build(set3_Cubemap, set3_CubemapLayout);
}