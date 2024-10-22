#include "ShadowPipeline.hpp"

#include "backend/VulkanContext.hpp"
#include "backend/pipeline/VulkanGraphicsPipelineBuilder.hpp"
#include "backend/shader/VulkanShader.h"

#include "ObjectPipeline.hpp"

#include "core/Time.hpp"

#include "renderer/scene/SceneManager.hpp"
#include "renderer/components/CameraComponent.hpp"
#include "renderer/vertices/Vertex.hpp"
#include "renderer/materials/Material.hpp"
#include "renderer/object/Mesh.hpp"

ShadowPipeline::ShadowPipeline()
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

	m_BufferOffscreenUniform.Destroy();

	m_Allocator.Cleanup();

	if (m_PipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(VulkanContext::GetDevice(), m_PipelineLayout, nullptr);
		m_PipelineLayout = VK_NULL_HANDLE;
	}
}

// Set up a separate render pass for the offscreen frame buffer
void ShadowPipeline::CreateRenderPass()
{
	// resize offscreen passes
	const auto& lights = SceneManager::Get()->getScene()->getLightInstances();
	offscreenpasses.resize(lights.size());

	for (int i = 0; i < lights.size(); ++i)
	{
		offscreenpasses[i].width = shadowMapize;
		offscreenpasses[i].height = shadowMapize;

		offscreenpasses[i].depthAttachment = std::make_unique<ImageDepth>(glm::uvec2{ shadowMapize, shadowMapize }, VK_FORMAT_D16_UNORM);

		// create render pass
		{
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

			VkRenderPassCreateInfo create_info{
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
				.attachmentCount = 1,
				.pAttachments = &attachmentDescription,
				.subpassCount = 1,
				.pSubpasses = &subpass,
				.dependencyCount = uint32_t(dependencies.size()),
				.pDependencies = dependencies.data(),
			};

			VulkanContext::VK_CHECK(
				vkCreateRenderPass(VulkanContext::GetDevice(), &create_info, nullptr, &offscreenpasses[i].renderPass),
				"[Vulkan] Create Render pass failed"
			);
		}

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

void ShadowPipeline::Rebuild()
{
}

void ShadowPipeline::CreatePipeline()
{
	CreateRenderPass();
	CreateDescriptors();
	CreatePipelineLayout();
	CreateGraphicsPipeline();
}

void ShadowPipeline::Prepare(const Scene* scene, const CommandBuffer& commandBuffer)
{
}

void ShadowPipeline::Render(const Scene* scene, const CommandBuffer& commandBuffer)
{
	const auto& lights = SceneManager::Get()->getScene()->getLightInstances();
	for (int lightIndex = 0; lightIndex < lights.size(); ++lightIndex)
	{
		BeginRenderPass(commandBuffer, lightIndex);

		// bind pipeline and descriptor sets
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenpasses[lightIndex].pipeline);
		UpdateAndBindDescriptors(commandBuffer, lightIndex);

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

void ShadowPipeline::CreateDescriptors()
{
	m_BufferOffscreenUniform = Buffer(
		sizeof(UniformDataOffscreen),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		Buffer::Mapped
	);

	// builds the uniform buffer
	VkDescriptorBufferInfo bufferInfo
	{
		.buffer = m_BufferOffscreenUniform.getBuffer(),
		.offset = 0,
		.range = m_BufferOffscreenUniform.getSize(),
	};

	DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_Allocator)
		.BindBuffer(0, &bufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		.Build(set0_Offscreen, set0_OffscreenLayout);
}

void ShadowPipeline::CreatePipelineLayout()
{
	std::array< VkDescriptorSetLayout, 1 > layouts{
		set0_OffscreenLayout
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

void ShadowPipeline::CreateGraphicsPipeline()
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

	VulkanGraphicsPipelineBuilder builder = VulkanGraphicsPipelineBuilder::Start()
		.SetDynamicStates(dynamic_states)
		.SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.SetColorBlending((uint32_t)attachment_states.size(), attachment_states.data());


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

void ShadowPipeline::UpdateAndBindDescriptors(const CommandBuffer& commandBuffer, uint32_t index)
{
	const auto& lights = SceneManager::Get()->getScene()->getLightInstances();
	auto& lightInfo = lights[index]->GetLightInfo();

	// update offscreen uniform
	glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(lightInfo.lightFOV), 1.0f, lightInfo.zNear, lightInfo.zFar);
	glm::mat4 depthViewMatrix = glm::lookAt(glm::vec3(lightInfo.position), glm::vec3(lightInfo.direction), Vec3::Up);
	m_OffscreenUniform.depthMVP = depthProjectionMatrix * depthViewMatrix;
	
	// set info lightspace
	lightInfo.lightspace = m_OffscreenUniform.depthMVP;

	memcpy(m_BufferOffscreenUniform.data(), &m_OffscreenUniform, sizeof(m_OffscreenUniform));

	// bind descriptors
	std::array< VkDescriptorSet, 1 > descriptor_sets{
		set0_Offscreen,
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