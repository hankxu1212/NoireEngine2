#include "ShadowPipeline.hpp"

#include "backend/VulkanContext.hpp"
#include "backend/pipeline/VulkanGraphicsPipelineBuilder.hpp"
#include "backend/shader/VulkanShader.h"

#include "ObjectPipeline.hpp"

#include "core/Time.hpp"
#include "utils/Logger.hpp"

#include "renderer/scene/SceneManager.hpp"
#include "renderer/components/CameraComponent.hpp"
#include "renderer/vertices/Vertex.hpp"
#include "renderer/materials/Material.hpp"
#include "renderer/object/Mesh.hpp"

// Depth bias (and slope) are used to avoid shadowing artifacts
// Constant depth bias factor (always applied)
const float depthBiasConstant = 1.25f;
// Slope depth bias factor, applied depending on polygon's slope
const float depthBiasSlope = 1.75f;

// Shadow map dimension
#if defined(__ANDROID__)
	// Use a smaller size on Android for performance reasons
const uint32_t shadowMapize{ 1024 };
#else
const uint32_t shadowMapize{ 2048 };
#endif

ShadowPipeline::ShadowPipeline(ObjectPipeline* objectPipeline) :
	p_ObjectPipeline(objectPipeline)
{
}

ShadowPipeline::~ShadowPipeline()
{
	for (int i = 0; i < offscreenpasses.size(); i++)
	{
		if (offscreenpasses[i].frameBuffer != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(VulkanContext::GetDevice(), offscreenpasses[i].frameBuffer, nullptr);
			offscreenpasses[i].frameBuffer = VK_NULL_HANDLE;
		}

		if (offscreenpasses[i].renderPass != VK_NULL_HANDLE) {
			vkDestroyRenderPass(VulkanContext::GetDevice(), offscreenpasses[i].renderPass, nullptr);
			offscreenpasses[i].renderPass = VK_NULL_HANDLE;
		}

		if (offscreenpasses[i].pipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(VulkanContext::GetDevice(), offscreenpasses[i].pipeline, nullptr);
			offscreenpasses[i].pipeline = VK_NULL_HANDLE;
		}
	}

	for (Workspace& workspace : workspaces)
	{
		workspace.LightSpaces.Destroy();
		workspace.LightSpaces_Src.Destroy();
	}
	workspaces.clear();

	m_Allocator.Cleanup();

	if (m_PipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(VulkanContext::GetDevice(), m_PipelineLayout, nullptr);
		m_PipelineLayout = VK_NULL_HANDLE;
	}
}

// Set up a separate render pass for the offscreen frame buffer
void ShadowPipeline::CreateRenderPass()
{
	CreateRenderPasses();
}

void ShadowPipeline::Rebuild()
{
}

void ShadowPipeline::CreatePipeline()
{
	CreateDescriptors();
	CreatePipelineLayout();
	CreateGraphicsPipeline();
}

void ShadowPipeline::Prepare(const Scene* scene, const CommandBuffer& commandBuffer)
{
	Workspace& workspace = workspaces[CURR_FRAME];

	const auto& lights = SceneManager::Get()->getScene()->getLightInstances();
	
	size_t needed_bytes = lights.size() * sizeof(Push);

	// resize as neccesary
	if (workspace.LightSpaces_Src.getBuffer() == VK_NULL_HANDLE
		|| workspace.LightSpaces_Src.getSize() < needed_bytes)
	{
		//round to next multiple of 4k to avoid re-allocating continuously if vertex count grows slowly:
		size_t new_bytes = ((needed_bytes + 4096) / 4096) * 4096;

		workspace.LightSpaces_Src.Destroy();
		workspace.LightSpaces.Destroy();

		workspace.LightSpaces_Src = Buffer(
			new_bytes,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, //going to have GPU copy from this memory
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //host-visible memory, coherent (no special sync needed)
			Buffer::Mapped //get a pointer to the memory
		);
		workspace.LightSpaces = Buffer(
			new_bytes,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, //going to use as storage buffer, also going to have GPU into this memory
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT //GPU-local memory
		);

		//update the descriptor set:
		VkDescriptorBufferInfo Lightspaces_info{
			.buffer = workspace.LightSpaces.getBuffer(),
			.offset = 0,
			.range = workspace.LightSpaces.getSize(),
		};

		DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_Allocator)
			.BindBuffer(0, &Lightspaces_info, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.Write(workspace.set0_Offscreen);

		NE_INFO("Reallocated SHADOWS to {} bytes", new_bytes);
	}

	assert(workspace.LightSpaces_Src.getSize() == workspace.LightSpaces.getSize());
	assert(workspace.LightSpaces_Src.getSize() >= needed_bytes);

	{ //copy LightSpaces into LightSpaces_Src:
		assert(workspace.LightSpaces_Src.data() != nullptr);

		size_t offset = 0;
		for (int i = 0; i < lights.size(); ++i) 
		{
			auto& lightInfo = lights[i]->GetLightInfo();
			memcpy(PTR_ADD(workspace.LightSpaces_Src.data(), offset), glm::value_ptr(lightInfo.lightspace), sizeof(glm::mat4));
			offset += sizeof(glm::mat4);
		}
	}

	Buffer::CopyBuffer(commandBuffer, workspace.LightSpaces_Src.getBuffer(), workspace.LightSpaces.getBuffer(), workspace.LightSpaces_Src.getSize());
}

void ShadowPipeline::Render(const Scene* scene, const CommandBuffer& commandBuffer)
{
	const auto& lights = SceneManager::Get()->getScene()->getLightInstances();

	// bind descriptors
	std::array< VkDescriptorSet, 2 > descriptor_sets{
		workspaces[CURR_FRAME].set0_Offscreen,
		p_ObjectPipeline->workspaces[CURR_FRAME].set1_StorageBuffers // for transforms
	};

	vkCmdBindDescriptorSets(
		commandBuffer, //command buffer
		VK_PIPELINE_BIND_POINT_GRAPHICS, //pipeline bind point
		m_PipelineLayout, //pipeline layout
		0, //first set
		uint32_t(descriptor_sets.size()), descriptor_sets.data(), //descriptor sets count, ptr
		0, nullptr //dynamic offsets count, ptr
	);

	// render each shadow map
	for (int lightIndex = 0; lightIndex < lights.size(); ++lightIndex)
	{
		BeginRenderPass(commandBuffer, lightIndex);

		// bind pipeline
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenpasses[lightIndex].pipeline);

		// push constant
		Push push{
			.lightspaceID = lightIndex
		};
		vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Push), &push);
		
		// draw once
		const auto& allInstances = scene->getObjectInstances();

		VkDrawIndexedIndirectCommand* drawCommands = (VkDrawIndexedIndirectCommand*)VulkanContext::Get()->getIndirectBuffer()->data();

		//draw all instances in relation to a certain material:
		uint32_t instanceIndex = 0;
		uint32_t offsetIndex = 0;
		VertexInput* previouslyBindedVertex = nullptr;

		for (int workflowIndex = 0; workflowIndex < allInstances.size(); ++workflowIndex)
		{
			auto& workflowInstances = allInstances[workflowIndex];
			if (workflowInstances.empty())
				continue;

			// make draws into compact batches
			std::vector<ObjectPipeline::IndirectBatch> draws = ObjectPipeline::CompactDraws(workflowInstances);

			//encode the draw data of each object into the indirect draw buffer
			uint32_t instanceCount = (uint32_t)workflowInstances.size();
			for (uint32_t i = 0; i < instanceCount; i++)
			{
				drawCommands[instanceIndex].indexCount = workflowInstances[i].mesh->getIndexCount();
				drawCommands[instanceIndex].instanceCount = 1;
				drawCommands[instanceIndex].firstIndex = 0;
				drawCommands[instanceIndex].vertexOffset = 0;
				drawCommands[instanceIndex].firstInstance = instanceIndex;
				instanceIndex++;
			}

			// draw each batch
			for (ObjectPipeline::IndirectBatch& draw : draws)
			{
				VertexInput* vertexInputPtr = draw.mesh->getVertexInput();
				if (vertexInputPtr != previouslyBindedVertex) {
					vertexInputPtr->Bind(commandBuffer);
					previouslyBindedVertex = vertexInputPtr;
				}

				draw.mesh->Bind(commandBuffer);

				constexpr uint32_t stride = sizeof(VkDrawIndexedIndirectCommand);
				VkDeviceSize offset = (draw.firstInstanceIndex + offsetIndex) * stride;

				vkCmdDrawIndexedIndirect(commandBuffer, VulkanContext::Get()->getIndirectBuffer()->getBuffer(), offset, draw.count, stride);
			}
			offsetIndex += instanceCount;
		}

		vkCmdEndRenderPass(commandBuffer);
	}
}

void ShadowPipeline::CreateRenderPasses()
{
	// resize offscreen passes
	const auto& lights = SceneManager::Get()->getScene()->getLightInstances();
	VkAttachmentDescription attachmentDescription{};
	attachmentDescription.format = VK_FORMAT_D16_UNORM;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear depth at beginning of the render pass
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;						// We will read from depth, so it's important to store the depth attachment results
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;					// We don't care about initial layout of the attachment
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;// Attachment will be transitioned to shader read at render pass end

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 0;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;			// Attachment will be used as depth/stencil during render pass

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 0;													// No color attachments
	subpass.pDepthStencilAttachment = &depthReference;									// Reference to our depth attachment

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderpassCreateInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &attachmentDescription,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = uint32_t(dependencies.size()),
		.pDependencies = dependencies.data(),
	};

	offscreenpasses.resize(lights.size());

	for (int i = 0; i < lights.size(); ++i)
	{
		offscreenpasses[i].width = shadowMapize;
		offscreenpasses[i].height = shadowMapize;

		offscreenpasses[i].depthAttachment = std::make_unique<ImageDepth>(glm::uvec2{ shadowMapize, shadowMapize }, VK_FORMAT_D16_UNORM);

		// create render pass
		VulkanContext::VK_CHECK(
			vkCreateRenderPass(VulkanContext::GetDevice(), &renderpassCreateInfo, nullptr, &offscreenpasses[i].renderPass),
			"[Vulkan] Create Render pass failed"
		);

		// create frame buffer
		{
			VkImageView view = offscreenpasses[i].depthAttachment->getView();

			VkFramebufferCreateInfo create_info
			{
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = offscreenpasses[i].renderPass,
				.attachmentCount = 1,
				.pAttachments = &view,
				.width = uint32_t(offscreenpasses[i].width),
				.height = uint32_t(offscreenpasses[i].height),
				.layers = 1,
			};

			VulkanContext::VK_CHECK(
				vkCreateFramebuffer(VulkanContext::GetDevice(), &create_info, nullptr, &offscreenpasses[i].frameBuffer),
				"[vulkan] Creating frame buffer failed"
			);
		}
	}
}

void ShadowPipeline::CreateDescriptors()
{
	workspaces.resize(VulkanContext::Get()->getFramesInFlight());

	for (Workspace& workspace : workspaces)
	{
		DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_Allocator)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.Build(workspace.set0_Offscreen, set0_OffscreenLayout);
	}
}

void ShadowPipeline::CreatePipelineLayout()
{
	std::array< VkDescriptorSetLayout, 2 > layouts{
		set0_OffscreenLayout,
		p_ObjectPipeline->set1_StorageBuffersLayout
	};

	//setup push constants
	VkPushConstantRange push_constant{
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof(Push),
	};

	VkPipelineLayoutCreateInfo create_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = uint32_t(layouts.size()),
		.pSetLayouts = layouts.data(),
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &push_constant,
	};

	VulkanContext::VK_CHECK(vkCreatePipelineLayout(VulkanContext::GetDevice(), &create_info, nullptr, &m_PipelineLayout));
}

void ShadowPipeline::CreateGraphicsPipeline()
{
	std::vector< VkDynamicState > dynamic_states{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_VERTEX_INPUT_EXT
	};

	VulkanGraphicsPipelineBuilder builder = VulkanGraphicsPipelineBuilder::Start()
		.SetDynamicStates(dynamic_states)
		.SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.SetRasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

	VulkanShader vertModule("../spv/shaders/shadow/offscreen.vert.spv", VulkanShader::ShaderStage::Vertex);
	VulkanShader fragModule("../spv/shaders/shadow/offscreen.frag.spv", VulkanShader::ShaderStage::Frag);

	std::array< VkPipelineShaderStageCreateInfo, 2 > stages{
		vertModule.shaderStage(),
		fragModule.shaderStage()
	};

	VkGraphicsPipelineCreateInfo create_info{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = uint32_t(stages.size()),
		.pStages = stages.data(),
		.pVertexInputState = builder.vertexInput,
		.pInputAssemblyState = &builder.inputAssembly,
		.pViewportState = &builder.viewportState,
		.pRasterizationState = &builder.rasterizer,
		.pMultisampleState = &builder.multisampling,
		.pDepthStencilState = &builder.depthStencil,
		.pColorBlendState = &builder.colorBlending,
		.pDynamicState = &builder.dynamicState,
		.layout = m_PipelineLayout,
		.renderPass = VK_NULL_HANDLE,
		.subpass = 0,
	};
	
	const auto& lights = SceneManager::Get()->getScene()->getLightInstances();

	// builds all pipelines
	for (int i = 0; i < lights.size(); ++i)
	{
		create_info.renderPass = offscreenpasses[i].renderPass;
		VulkanContext::VK_CHECK(
			vkCreateGraphicsPipelines(VulkanContext::GetDevice(), VulkanContext::Get()->getPipelineCache(), 1, &create_info, nullptr, &offscreenpasses[i].pipeline),
			"[Vulkan] Create pipeline failed");
	}
}

void ShadowPipeline::BeginRenderPass(const CommandBuffer& commandBuffer, uint32_t index)
{
	static std::array< VkClearValue, 1 > clear_values{
		VkClearValue{.depthStencil{.depth = 1.0f, .stencil = 0 } },
	};

	VkRenderPassBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = offscreenpasses[index].renderPass,
		.framebuffer = offscreenpasses[index].frameBuffer,
		.renderArea{
			.offset = {.x = 0, .y = 0},
			.extent = {.width = (uint32_t)offscreenpasses[index].width, .height = (uint32_t)offscreenpasses[index].height},
		},
		.clearValueCount = 1,
		.pClearValues = clear_values.data(),
	};

	vkCmdBeginRenderPass(commandBuffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

	VkRect2D scissor{
		.offset = {.x = 0, .y = 0},
		.extent = {.width = (uint32_t)offscreenpasses[index].width, .height = (uint32_t)offscreenpasses[index].height},
	};
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	VkViewport viewport{
		.x = 0.0f,
		.y = 0.0f,
		.width = float(offscreenpasses[index].width),
		.height = float(offscreenpasses[index].height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	// Set depth bias (aka "Polygon offset")
	// Required to avoid shadow mapping artifacts
	vkCmdSetDepthBias(
		commandBuffer,
		depthBiasConstant,
		0.0f,
		depthBiasSlope);
}