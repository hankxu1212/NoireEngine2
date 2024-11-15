#include "GizmosPipeline.hpp"
#include "backend/shader/VulkanShader.h"
#include "backend/VulkanContext.hpp"
#include "renderer/vertices/PosColVertex.hpp"
#include "renderer/components/CameraComponent.hpp"
#include "renderer/scene/Scene.hpp"
#include "backend/pipeline/VulkanGraphicsPipelineBuilder.hpp"
#include "renderer/object/Mesh.hpp"

GizmosPipeline::~GizmosPipeline()
{
	for (Workspace& workspace : workspaces)
	{
		workspace.LinesVerticesSrc.Destroy();
		workspace.LinesVertices.Destroy();
		
		workspace.Camera.Destroy();

		workspace.TransformsSrc.Destroy();
		workspace.Transforms.Destroy();
	}
	workspaces.clear();

	m_DescriptorAllocator.Cleanup(); // destroy pool and sets

	if (m_LinesPipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(VulkanContext::GetDevice(), m_LinesPipelineLayout, nullptr);
		m_LinesPipelineLayout = VK_NULL_HANDLE;
	}

	if (m_LinesPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(VulkanContext::GetDevice(), m_LinesPipeline, nullptr);
		m_LinesPipeline = VK_NULL_HANDLE;
	}

	if (m_OutlinePipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(VulkanContext::GetDevice(), m_OutlinePipelineLayout, nullptr);
		m_OutlinePipelineLayout = VK_NULL_HANDLE;
	}

	if (m_OutlinePipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(VulkanContext::GetDevice(), m_OutlinePipeline, nullptr);
		m_OutlinePipeline = VK_NULL_HANDLE;
	}
}

void GizmosPipeline::CreatePipeline()
{
	workspaces.resize(VulkanContext::Get()->getFramesInFlight());

	CreateLinesDescriptors();
	CreateLinesPipelineLayout();
	CreateLinesGraphicsPipeline();

	CreateOutlineDescriptors();
	CreateOutlinePipelineLayout();
	CreateOutlineGraphicsPipeline();
}

static uint32_t totalGizmosSize;

void GizmosPipeline::Render(const Scene* scene, const CommandBuffer& commandBuffer)
{
	// bind descriptor set
	{
		Workspace& workspace = workspaces[CURR_FRAME];

		std::array< VkDescriptorSet, 1 > descriptor_sets{
			workspace.set0,
		};
		vkCmdBindDescriptorSets(
			commandBuffer, //command buffer
			VK_PIPELINE_BIND_POINT_GRAPHICS, //pipeline bind point
			m_LinesPipelineLayout, //pipeline layout
			0, //first set
			uint32_t(descriptor_sets.size()), descriptor_sets.data(), //descriptor sets count, ptr
			0, nullptr //dynamic offsets count, ptr
		);
	}

	RenderLines(scene, commandBuffer);
	RenderOutline(scene, commandBuffer);
}

void GizmosPipeline::RenderLines(const Scene* scene, const CommandBuffer& commandBuffer)
{
	if (scene->getGizmosInstances().empty())
		return;

	Workspace& workspace = workspaces[CURR_FRAME];

	{ //draw with the lines pipeline:
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LinesPipeline);

		{ //use lines_vertices (offset 0) as vertex buffer binding 0:
			std::array< VkBuffer, 1 > vertex_buffers{ workspace.LinesVertices.getBuffer() };
			std::array< VkDeviceSize, 1 > offsets{ 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, uint32_t(vertex_buffers.size()), vertex_buffers.data(), offsets.data());
		}

		//draw lines vertices:
		vkCmdDraw(commandBuffer, totalGizmosSize, 1, 0, 0);
	}
}

void GizmosPipeline::RenderOutline(const Scene* scene, const CommandBuffer& commandBuffer)
{
	const auto& selectedInstances = scene->getSelectedObjectInstances();
	if (selectedInstances.empty())
		return;

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_OutlinePipeline);

	for (const auto& instance : selectedInstances) 
	{
		instance.mesh->getVertexInput()->Bind(commandBuffer);
		instance.mesh->Bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, instance.mesh->getIndexCount(), 1, 0, 0, 0);
	}
}

void GizmosPipeline::Prepare(const Scene* scene, const CommandBuffer& commandBuffer)
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

		if (workspace.LinesVerticesSrc.getBuffer() == VK_NULL_HANDLE || workspace.LinesVerticesSrc.getSize() < needed_bytes)
		{
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
		memcpy(workspace.Camera.data(), &pushCamera, sizeof(CameraUniform));
	}

	{
		const auto& sceneObjectInstances = scene->getSelectedObjectInstances();

		size_t needed_bytes = sceneObjectInstances.size() * sizeof(ObjectInstance::TransformUniform);

		// resize as neccesary
		if (workspace.TransformsSrc.getBuffer() == VK_NULL_HANDLE
			|| workspace.TransformsSrc.getSize() < needed_bytes)
		{
			//round to next multiple of 4k to avoid re-allocating continuously if vertex count grows slowly:
			size_t new_bytes = ((needed_bytes + 4096) / 4096) * 4096;

			workspace.TransformsSrc.Destroy();
			workspace.Transforms.Destroy();

			workspace.TransformsSrc = Buffer(
				new_bytes,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, //going to have GPU copy from this memory
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //host-visible memory, coherent (no special sync needed)
				Buffer::Mapped //get a pointer to the memory
			);
			workspace.Transforms = Buffer(
				new_bytes,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, //going to use as storage buffer, also going to have GPU into this memory
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT //GPU-local memory
			);

			//update the descriptor set:
			VkDescriptorBufferInfo Transforms_info{
				.buffer = workspace.Transforms.getBuffer(),
				.offset = 0,
				.range = workspace.Transforms.getSize(),
			};

			DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
				.BindBuffer(Set0::Transforms, &Transforms_info, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
				.Write(workspace.set0);

			NE_INFO("Reallocated transform to {} bytes", new_bytes);
		}

		assert(workspace.TransformsSrc.getSize() == workspace.Transforms.getSize());
		assert(workspace.TransformsSrc.getSize() >= needed_bytes);

		{ //copy transforms into Transforms_src:
			ObjectInstance::TransformUniform* start = reinterpret_cast<ObjectInstance::TransformUniform*>(workspace.TransformsSrc.data());
			ObjectInstance::TransformUniform* out = start;

			for (auto& inst : sceneObjectInstances) {
				*out = inst.m_TransformUniform;
				++out;
			}

			size_t regionSize = reinterpret_cast<char*>(out) - reinterpret_cast<char*>(start);
			Buffer::CopyBuffer(commandBuffer, workspace.TransformsSrc.getBuffer(), workspace.Transforms.getBuffer(), regionSize);
		}
	}
}

void GizmosPipeline::CreateLinesPipelineLayout()
{
	std::array< VkDescriptorSetLayout, 1 > layouts{
		set0_Layout,
	};

	VkPipelineLayoutCreateInfo create_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = uint32_t(layouts.size()),
		.pSetLayouts = layouts.data(),
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr,
	};

	VulkanContext::VK(vkCreatePipelineLayout(VulkanContext::GetDevice(), &create_info, nullptr, &m_LinesPipelineLayout));
}

void GizmosPipeline::CreateLinesGraphicsPipeline()
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
		.Build("../spv/shaders/lines.vert.spv", "../spv/shaders/lines.frag.spv", &m_LinesPipeline, m_LinesPipelineLayout, Renderer::Instance->GetUIRenderPass());
}

void GizmosPipeline::CreateLinesDescriptors()
{
	for (Workspace& workspace : workspaces)
	{
		workspace.Camera = Buffer(
			sizeof(CameraUniform),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			Buffer::Mapped
		);

		VkDescriptorBufferInfo CameraInfo = workspace.Camera.GetDescriptorInfo();
		DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
			.BindBuffer(Set0::World, &CameraInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.AddBinding(Set0::Transforms, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.Build(workspace.set0, set0_Layout);
	}
}

void GizmosPipeline::CreateOutlineGraphicsPipeline()
{
	std::vector< VkDynamicState > dynamic_states{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_VERTEX_INPUT_EXT
	};

	std::vector<VkPipelineColorBlendAttachmentState> blendStates{
		VkPipelineColorBlendAttachmentState{
			.blendEnable = VK_FALSE,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		},
	};

	VulkanGraphicsPipelineBuilder::Start()
		.SetDynamicStates(dynamic_states)
		.SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.SetColorBlending((uint32_t)blendStates.size(), blendStates.data())
		.SetRasterization(VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
		.Build("../spv/shaders/wireframe.vert.spv", "../spv/shaders/wireframe.frag.spv", &m_OutlinePipeline, m_OutlinePipelineLayout, Renderer::Instance->GetUIRenderPass());
}

void GizmosPipeline::CreateOutlinePipelineLayout()
{	
	std::vector< VkDescriptorSetLayout> layouts{
		set0_Layout
	};

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = uint32_t(layouts.size()),
		.pSetLayouts = layouts.data(),
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr,
	};

	VulkanContext::VK(vkCreatePipelineLayout(VulkanContext::GetDevice(), &pipelineLayoutCreateInfo, nullptr, &m_OutlinePipelineLayout));
}

void GizmosPipeline::CreateOutlineDescriptors()
{

}
