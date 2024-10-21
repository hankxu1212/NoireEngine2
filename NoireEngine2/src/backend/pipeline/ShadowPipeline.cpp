#include "ShadowPipeline.hpp"

#include "backend/VulkanContext.hpp"
#include "backend/pipeline/VulkanGraphicsPipelineBuilder.hpp"

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
	if (offscreenPass.frameBuffer != VK_NULL_HANDLE) {
		vkDestroyFramebuffer(VulkanContext::GetDevice(), offscreenPass.frameBuffer, nullptr);
		offscreenPass.frameBuffer = VK_NULL_HANDLE;
	}

	if (offscreenPass.renderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(VulkanContext::GetDevice(), offscreenPass.renderPass, nullptr);
		offscreenPass.renderPass = VK_NULL_HANDLE;
	}

	m_BufferOffscreenUniform.Destroy();

	m_Allocator.Cleanup();

	if (m_PipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(VulkanContext::GetDevice(), m_PipelineLayout, nullptr);
		m_PipelineLayout = VK_NULL_HANDLE;
	}

	if (m_Pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(VulkanContext::GetDevice(), m_Pipeline, nullptr);
		m_Pipeline = VK_NULL_HANDLE;
	}
}

// Set up a separate render pass for the offscreen frame buffer
void ShadowPipeline::CreateRenderPass()
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
		vkCreateRenderPass(VulkanContext::GetDevice(), &create_info, nullptr, &offscreenPass.renderPass),
		"[Vulkan] Create Render pass failed"
	);
}

void ShadowPipeline::Rebuild()
{
}

void ShadowPipeline::CreatePipeline()
{
	PrepareOffscreenFrameBuffer();
	PrepareUniformBuffers();
	CreateDescriptors();
	CreatePipelineLayout();
	CreateGraphicsPipeline();
}

void ShadowPipeline::Prepare(const Scene* scene, const CommandBuffer& commandBuffer)
{
	UpdateLight();
	UpdateUniforms();

	auto uniformDataScene = (Scene::SceneUniform*)scene->getSceneUniformPtr();
	uniformDataScene->lightPos = glm::vec4(lightPos, 1.0f);
	uniformDataScene->depthBiasMVP = m_OffscreenUniform.depthMVP;
	uniformDataScene->zNear = zNear;
	uniformDataScene->zFar = zFar;
}

void ShadowPipeline::Render(const Scene* scene, const CommandBuffer& commandBuffer)
{
	BeginRenderPass(commandBuffer);

	// bind pipeline and descriptor sets
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	BindDescriptors(commandBuffer);

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

void ShadowPipeline::PrepareOffscreenFrameBuffer()
{
	offscreenPass.width = shadowMapize;
	offscreenPass.height = shadowMapize;

	m_DepthAttachment = std::make_unique<ImageDepth>(glm::uvec2{ shadowMapize, shadowMapize }, VK_FORMAT_D16_UNORM);

	CreateRenderPass();

	VkImageView view = m_DepthAttachment->getView();

	VkFramebufferCreateInfo create_info
	{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = offscreenPass.renderPass,
		.attachmentCount = 1,
		.pAttachments = &view,
		.width = uint32_t(offscreenPass.width),
		.height = uint32_t(offscreenPass.height),
		.layers = 1,
	};

	VulkanContext::VK_CHECK(
		vkCreateFramebuffer(VulkanContext::GetDevice(), &create_info, nullptr, &offscreenPass.frameBuffer),
		"[vulkan] Creating frame buffer failed"
	);
}

void ShadowPipeline::PrepareUniformBuffers()
{
	m_BufferOffscreenUniform = Buffer(
		sizeof(UniformDataOffscreen),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		Buffer::Mapped
	);

	UpdateLight();
	UpdateUniforms();
}

void ShadowPipeline::UpdateUniforms()
{
	// update offscreen
	glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(lightFOV), 1.0f, zNear, zFar);
	glm::mat4 depthViewMatrix = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
	glm::mat4 depthModelMatrix = glm::mat4(1.0f);

	m_OffscreenUniform.depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

	memcpy(m_BufferOffscreenUniform.data(), &m_OffscreenUniform, sizeof(m_OffscreenUniform));
}

void ShadowPipeline::CreateDescriptors()
{
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

	VulkanGraphicsPipelineBuilder::Start()
		.SetDynamicStates(dynamic_states)
		.SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.SetColorBlending((uint32_t)attachment_states.size(), attachment_states.data())
		.Build("../spv/shaders/shadow/offscreen.vert.spv", "../spv/shaders/shadow/offscreen.frag.spv", &m_Pipeline, m_PipelineLayout, offscreenPass.renderPass);
}

void ShadowPipeline::BindDescriptors(const CommandBuffer& commandBuffer)
{
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

void ShadowPipeline::BeginRenderPass(const CommandBuffer& commandBuffer)
{
	static std::array< VkClearValue, 1 > clear_values{
		VkClearValue{.depthStencil{.depth = 1.0f, .stencil = 0 } },
	};

	VkRenderPassBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = offscreenPass.renderPass,
		.framebuffer = offscreenPass.frameBuffer,
		.renderArea{
			.offset = {.x = 0, .y = 0},
			.extent = {.width = (uint32_t)offscreenPass.width, .height = (uint32_t)offscreenPass.height},
		},
		.clearValueCount = 1,
		.pClearValues = clear_values.data(),
	};

	vkCmdBeginRenderPass(commandBuffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

	VkRect2D scissor{
		.offset = {.x = 0, .y = 0},
		.extent = {.width = (uint32_t)offscreenPass.width, .height = (uint32_t)offscreenPass.height},
	};
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	VkViewport viewport{
		.x = 0.0f,
		.y = 0.0f,
		.width = float(offscreenPass.width),
		.height = float(offscreenPass.height),
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

void ShadowPipeline::UpdateLight()
{
	// Animate the light source
	lightPos.x = cos(glm::radians(Time::Now * 360.0f)) * 40.0f;
	lightPos.y = -50.0f + sin(glm::radians(Time::Now * 360.0f)) * 20.0f;
	lightPos.z = 25.0f + sin(glm::radians(Time::Now * 360.0f)) * 5.0f;
}
