#include "LinesPipeline.hpp"
#include "backend/shader/VulkanShader.h"
#include "backend/VulkanContext.hpp"
#include "renderer/vertices/PosColVertex.hpp"
#include "renderer/components/CameraComponent.hpp"
#include "renderer/scene/Scene.hpp"
#include "backend/pipeline/VulkanGraphicsPipelineBuilder.hpp"

LinesPipeline::~LinesPipeline()
{
	for (Workspace& workspace : workspaces)
	{
		workspace.LinesVerticesSrc.Destroy();
		workspace.LinesVertices.Destroy();
		workspace.CameraSrc.Destroy();
		workspace.Camera.Destroy();
	}
	workspaces.clear();

	m_DescriptorAllocator.Cleanup(); // destroy pool and sets

	if (m_PipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(VulkanContext::GetDevice(), m_PipelineLayout, nullptr);
		m_PipelineLayout = VK_NULL_HANDLE;
	}

	if (m_Pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(VulkanContext::GetDevice(), m_Pipeline, nullptr);
		m_Pipeline = VK_NULL_HANDLE;
	}
}

void LinesPipeline::CreatePipeline()
{
	workspaces.resize(VulkanContext::Get()->getFramesInFlight());

	CreateDescriptors();
	CreatePipelineLayout();
	CreateGraphicsPipeline();
}

static uint32_t totalGizmosSize;

void LinesPipeline::Render(const Scene* scene, const CommandBuffer& commandBuffer)
{
	if (scene->getGizmosInstances().empty())
		return;

	Workspace& workspace = workspaces[CURR_FRAME];

	{ //draw with the lines pipeline:
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

		{ //use lines_vertices (offset 0) as vertex buffer binding 0:
			std::array< VkBuffer, 1 > vertex_buffers{ workspace.LinesVertices.getBuffer()};
			std::array< VkDeviceSize, 1 > offsets{ 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, uint32_t(vertex_buffers.size()), vertex_buffers.data(), offsets.data());
		}

		{ //bind Camera descriptor set:
			std::array< VkDescriptorSet, 1 > descriptor_sets{
				workspace.set0_Camera, //0: Camera
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

		//draw lines vertices:
		vkCmdDraw(commandBuffer, totalGizmosSize, 1, 0, 0);
	}
}

void LinesPipeline::Prepare(const Scene* scene, const CommandBuffer& commandBuffer)
{
	Workspace& workspace = workspaces[CURR_FRAME];

	const std::vector<GizmosInstance*>& gizmos = scene->getGizmosInstances();
	
	//upload lines vertices:
	if (!gizmos.empty()) 
	{
		//[re-]allocate lines buffers if needed:
		size_t needed_bytes = 0;
		for (int i = 0; i < gizmos.size(); ++i)
			needed_bytes += gizmos[i]->m_LinesVertices.size() * sizeof(PosColVertex);
		totalGizmosSize = uint32_t(needed_bytes / sizeof(PosColVertex));

		if (workspace.LinesVerticesSrc.getBuffer() == VK_NULL_HANDLE || workspace.LinesVerticesSrc.getSize() < needed_bytes) {
			//round to next multiple of 4k to avoid re-allocating continuously if vertex count grows slowly:
			size_t new_bytes = ((needed_bytes + 4096) / 4096) * 4096;

			workspace.LinesVerticesSrc.Destroy();
			workspace.LinesVertices.Destroy();

			workspace.LinesVerticesSrc = Buffer(
				new_bytes,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, //going to have GPU copy from this memory
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //host-visible memory, coherent (no special sync needed)
				Buffer::Mapped //get a pointer to the memory
			);
			workspace.LinesVertices = Buffer(
				new_bytes,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, //going to use as vertex buffer, also going to have GPU into this memory
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT //GPU-local memory
			);

			NE_INFO("Re-allocated lines buffers to {} bytes ", new_bytes);
		}

		assert(workspace.LinesVerticesSrc.getSize() >= needed_bytes);

		//host-side copy into LinesVerticesSrc:
		size_t offset = 0;
		for (int i = 0; i < gizmos.size(); ++i) {
			auto size = gizmos[i]->m_LinesVertices.size() * sizeof(PosColVertex);
			memcpy(
				PTR_ADD(workspace.LinesVerticesSrc.data(), offset),
				gizmos[i]->m_LinesVertices.data(), 
				size
			);
			offset += size;
		}
		Buffer::CopyBuffer(commandBuffer, workspace.LinesVerticesSrc.getBuffer(), workspace.LinesVertices.getBuffer(), needed_bytes);
	}

	{ //upload camera info:
		pushCamera.clipFromWorld = scene->GetRenderCam()->camera()->getWorldToClipMatrix();
		Buffer::CopyFromHost(commandBuffer, workspace.CameraSrc, workspace.Camera, sizeof(CameraUniform), &pushCamera);
	}
}

void LinesPipeline::CreatePipelineLayout()
{
	std::array< VkDescriptorSetLayout, 1 > layouts{
		set0_CameraLayout,
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

void LinesPipeline::CreateGraphicsPipeline()
{
	std::vector< VkDynamicState > dynamic_states{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	std::array< VkPipelineColorBlendAttachmentState, 1 > attachment_states{
		VkPipelineColorBlendAttachmentState{
			.blendEnable = VK_FALSE,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		}
	};

	VulkanGraphicsPipelineBuilder::Start()
		.SetDynamicStates(dynamic_states)
		.SetVertexInput(&PosColVertex::array_input_state)
		.SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
		.SetRasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 2.0f)
		.SetColorBlending((uint32_t)attachment_states.size(), attachment_states.data())
		.Build("../spv/shaders/lines.vert.spv", "../spv/shaders/lines.frag.spv", &m_Pipeline, m_PipelineLayout, Renderer::Instance->s_CompositionPass->renderpass);
}

void LinesPipeline::CreateDescriptors()
{
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

		VkDescriptorBufferInfo CameraInfo = workspace.Camera.GetDescriptorInfo();
		DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
			.BindBuffer(0, &CameraInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.Build(workspace.set0_Camera, set0_CameraLayout);
	}
}
