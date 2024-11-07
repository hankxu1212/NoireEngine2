#include "SkyboxPipeline.hpp"
#include "backend/shader/VulkanShader.h"
#include "backend/VulkanContext.hpp"
#include "renderer/vertices/PosColVertex.hpp"
#include "renderer/components/CameraComponent.hpp"
#include "renderer/scene/Scene.hpp"
#include "core/resources/Files.hpp"
#include "renderer/vertices/PosVertex.hpp"
#include "backend/pipeline/VulkanGraphicsPipelineBuilder.hpp"
#include "renderer/scene/SceneManager.hpp"

float skyboxVertices[] = {
	// positions          
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	-1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f
};
#define SKYBOX_SIZE_BYTES 36 * 12
static_assert(sizeof(skyboxVertices) == SKYBOX_SIZE_BYTES);

SkyboxPipeline::SkyboxPipeline(Renderer* renderer) :
	p_ObjectPipeline(renderer)
{
}

SkyboxPipeline::~SkyboxPipeline()
{
	for (Workspace& workspace : workspaces)
	{
		workspace.CameraSrc.Destroy();
		workspace.Camera.Destroy();
	}
	workspaces.clear();

	m_DescriptorAllocator.Cleanup(); // destroy pool and sets

	m_VertexBuffer.Destroy();

	if (m_PipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(VulkanContext::GetDevice(), m_PipelineLayout, nullptr);
		m_PipelineLayout = VK_NULL_HANDLE;
	}

	if (m_Pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(VulkanContext::GetDevice(), m_Pipeline, nullptr);
		m_Pipeline = VK_NULL_HANDLE;
	}
}

void SkyboxPipeline::CreatePipeline()
{
	workspaces.resize(VulkanContext::Get()->getFramesInFlight());
	CreateDescriptors();
	CreateVertexBuffer();
	CreatePipelineLayout();
	CreateGraphicsPipeline();
}

void SkyboxPipeline::Render(const Scene* scene, const CommandBuffer& commandBuffer)
{
	Workspace& workspace = workspaces[CURR_FRAME];

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

	std::array< VkBuffer, 1 > vertex_buffers{ m_VertexBuffer.getBuffer() };
	std::array< VkDeviceSize, 1 > offsets{ 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertex_buffers.data(), offsets.data());

	{ //bind Camera descriptor set:
		std::array< VkDescriptorSet, 2 > descriptor_sets{
			workspace.set0_Camera,
			p_ObjectPipeline->set3_IBL
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

	vkCmdDraw(commandBuffer, 36, 1, 0, 0);
}

void SkyboxPipeline::Prepare(const Scene* scene, const CommandBuffer& commandBuffer)
{
	Workspace& workspace = workspaces[CURR_FRAME];

	{ //upload camera info:
		SkyboxPipeline::CameraUniform camera{
			.projection = scene->GetRenderCam()->camera()->getProjectionMatrix(),
			.view = scene->GetRenderCam()->camera()->getViewMatrix()
		};

		//host-side copy into Camera_src:
		memcpy(workspace.CameraSrc.data(), &camera, sizeof(camera));

		Buffer::CopyBuffer(commandBuffer, workspace.CameraSrc.getBuffer(), workspace.Camera.getBuffer(), sizeof(camera));
	}
}

void SkyboxPipeline::CreateGraphicsPipeline()
{
	std::vector< VkDynamicState > dynamic_states{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
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
		.SetVertexInput(&PosVertex::array_input_state)
		.SetRasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_CLOCKWISE)
		.SetDepthStencil(VK_TRUE, VK_TRUE, VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL)
		.SetColorBlending((uint32_t)attachment_states.size(), attachment_states.data())
		.Build("../spv/shaders/skybox.vert.spv", "../spv/shaders/skybox.frag.spv", &m_Pipeline, m_PipelineLayout, p_ObjectPipeline->m_Renderpass->renderpass);
}

void SkyboxPipeline::CreatePipelineLayout()
{
	std::array< VkDescriptorSetLayout, 2 > layouts{
		set0_CameraLayout,
		p_ObjectPipeline->set3_IBLLayout
	};

	VkPipelineLayoutCreateInfo create_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = uint32_t(layouts.size()),
		.pSetLayouts = layouts.data(),
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr,
	};

	VulkanContext::VK(vkCreatePipelineLayout(VulkanContext::GetDevice(), &create_info, nullptr, &m_PipelineLayout));
}

void SkyboxPipeline::CreateDescriptors()
{
	// create world and transform descriptor
	for (Workspace& workspace : workspaces)
	{
		workspace.CameraSrc = Buffer(
			sizeof(CameraUniform),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			Buffer::Mapped
		);
		workspace.Camera = Buffer(
			sizeof(CameraUniform),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		VkDescriptorBufferInfo CameraInfo
		{
			.buffer = workspace.Camera.getBuffer(),
			.offset = 0,
			.range = workspace.Camera.getSize(),
		};

		DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
			.BindBuffer(0, &CameraInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.Build(workspace.set0_Camera, set0_CameraLayout);
	}
}

void SkyboxPipeline::CreateVertexBuffer()
{
	m_VertexBuffer = Buffer(
		SKYBOX_SIZE_BYTES,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	Buffer::TransferToBufferIdle(skyboxVertices, SKYBOX_SIZE_BYTES, m_VertexBuffer.getBuffer());
}
