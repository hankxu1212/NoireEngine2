#include "LinesPipeline.hpp"
#include "backend/shader/VulkanShader.h"
#include "backend/VulkanContext.hpp"
#include "renderer/vertices/PosColVertex.hpp"
#include "renderer/components/CameraComponent.hpp"
#include "renderer/scene/Scene.hpp"

static uint32_t vert_code[] =
#include "spv/shaders/lines.vert.inl"
;

static uint32_t frag_code[] =
#include "spv/shaders/lines.frag.inl"
;

LinesPipeline::LinesPipeline(ObjectPipeline* objectPipeline) :
	p_ObjectPipeline(objectPipeline)
{
}

LinesPipeline::~LinesPipeline()
{
	for (Workspace& workspace : workspaces)
	{
		workspace.CameraPersistent.Destroy();
		workspace.LinesVerticesPersistent.Destroy();
	}
	workspaces.clear();

	m_DescriptorAllocator.Cleanup();

	if (m_PipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(VulkanContext::GetDevice(), m_PipelineLayout, nullptr);
		m_PipelineLayout = VK_NULL_HANDLE;
	}

	if (m_Pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(VulkanContext::GetDevice(), m_Pipeline, nullptr);
		m_Pipeline = VK_NULL_HANDLE;
	}

	if (set0_CameraLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(VulkanContext::GetDevice(), set0_CameraLayout, nullptr);
		set0_CameraLayout = VK_NULL_HANDLE;
	}
}

void LinesPipeline::CreatePipeline()
{
	workspaces.resize(VulkanContext::Get()->getWorkspaceSize());

	CreateDescriptors();
	CreatePipelineLayout();
	CreateGraphicsPipeline();
}
static uint32_t totalGizmosSize;

void LinesPipeline::Render(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId)
{
	if (scene->getGizmosInstances().empty())
		return;

	Workspace& workspace = workspaces[surfaceId];

	{ //draw with the lines pipeline:
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

		{ //use lines_vertices (offset 0) as vertex buffer binding 0:
			std::array< VkBuffer, 1 > vertex_buffers{ workspace.LinesVerticesPersistent.getBuffer()};
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

void* OffsetPointer(void* ptr, size_t offset) {
	return static_cast<void*>(static_cast<std::byte*>(ptr) + offset);
}

void LinesPipeline::Prepare(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId)
{
	Workspace& workspace = workspaces[surfaceId];

	const std::vector<GizmosInstance*>& gizmos = scene->getGizmosInstances();
	
	if (!gizmos.empty()) { //upload lines vertices:
		//[re-]allocate lines buffers if needed:
		size_t needed_bytes = 0;
		for (int i = 0; i < gizmos.size(); ++i)
			needed_bytes += gizmos[i]->m_LinesVertices.size() * sizeof(PosColVertex);
		totalGizmosSize = uint32_t(needed_bytes / sizeof(PosColVertex));

		if (workspace.LinesVerticesPersistent.getBuffer() == VK_NULL_HANDLE
			|| workspace.LinesVerticesPersistent.getSize() < needed_bytes) {
			workspace.LinesVerticesPersistent.Destroy();

			if (workspace.LinesVerticesPersistent.getBuffer() == VK_NULL_HANDLE || workspace.LinesVerticesPersistent.getSize() < needed_bytes) {
				//round to next multiple of 4k to avoid re-allocating continuously if vertex count grows slowly:
				size_t new_bytes = ((needed_bytes + 4096) / 4096) * 4096;

				workspace.LinesVerticesPersistent = Buffer(
					new_bytes,
					VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, //going to use as vertex buffer, also going to have GPU into this memory
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					Buffer::Mapped
				);

				std::cout << "Re-allocated lines buffers to " << new_bytes << " bytes." << std::endl;
			}
		}

		assert(workspace.LinesVerticesPersistent.getSize() >= needed_bytes);

		//host-side copy into LinesVerticesSrc:
		size_t offset = 0;
		for (int i = 0; i < gizmos.size(); ++i) {
			auto size = gizmos[i]->m_LinesVertices.size() * sizeof(PosColVertex);
			std::memcpy(
				OffsetPointer(workspace.LinesVerticesPersistent.data(), offset),
				gizmos[i]->m_LinesVertices.data(), 
				size
			);
			offset += size;
		}
	}

	{ //upload camera info:
		LinesPipeline::CameraUniform camera{
			.clipFromWorld = scene->GetRenderCam()->camera()->getWorldToClipMatrix()
		};

		//host-side copy into Camera_src:
		memcpy(workspace.CameraPersistent.data(), &camera, sizeof(camera));
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

	VulkanContext::VK_CHECK(vkCreatePipelineLayout(VulkanContext::GetDevice(), &create_info, nullptr, &m_PipelineLayout));
}

void LinesPipeline::CreateGraphicsPipeline()
{
	VulkanShader vertModule(vert_code, VulkanShader::ShaderStage::Vertex);
	VulkanShader fragModule(frag_code, VulkanShader::ShaderStage::Frag);

	std::array< VkPipelineShaderStageCreateInfo, 2 > stages{
		vertModule.shaderStage(),
		fragModule.shaderStage()
	};

	//the viewport and scissor state will be set at runtime for the pipeline:
	std::vector< VkDynamicState > dynamic_states{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
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

	//this pipeline will draw lines:
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
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
		.lineWidth = 2.0f,
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
		.pVertexInputState = &PosColVertex::array_input_state,
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

	VulkanContext::VK_CHECK(vkCreateGraphicsPipelines(VulkanContext::GetDevice(), VK_NULL_HANDLE, 1, &create_info, nullptr, &m_Pipeline));
}

void LinesPipeline::CreateDescriptors()
{
	// create world and transform descriptor
	for (Workspace& workspace : workspaces)
	{
		workspace.CameraPersistent = Buffer(
			sizeof(CameraUniform),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			Buffer::Mapped
		);

		VkDescriptorBufferInfo CameraInfo
		{
			.buffer = workspace.CameraPersistent.getBuffer(),
			.offset = 0,
			.range = workspace.CameraPersistent.getSize(),
		};

		DescriptorBuilder::Start(&m_DescriptorLayoutCache, &m_DescriptorAllocator)
			.BindBuffer(0, &CameraInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.Build(workspace.set0_Camera, set0_CameraLayout);
	}
}
