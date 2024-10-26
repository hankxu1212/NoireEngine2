#include "ShadowPipeline.hpp"

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

//#include "glm/gtc/matrix_transform.hpp"

// Depth bias (and slope) are used to avoid shadowing artifacts
// Constant depth bias factor (always applied)
const float depthBiasConstant = 1.25f;
// Slope depth bias factor, applied depending on polygon's slope
const float depthBiasSlope = 1.75f;

#if defined(__ANDROID__)
#define SHADOWMAP_DIM 1024
#define CASCADED_SHADOWMAP_DIM 2048
#else
#define SHADOWMAP_DIM 2048
#define CASCADED_SHADOWMAP_DIM 4096
#endif

static std::array< VkClearValue, 1 > clear_values{
	VkClearValue{.depthStencil{.depth = 1.0f, .stencil = 0 } },
};

ShadowPipeline::ShadowPipeline(ObjectPipeline* objectPipeline) :
	p_ObjectPipeline(objectPipeline)
{
}

ShadowPipeline::~ShadowPipeline()
{
	for (int i = 0; i < m_ShadowMapPasses.size(); i++)
	{
		if (m_ShadowMapPasses[i].frameBuffer != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(VulkanContext::GetDevice(), m_ShadowMapPasses[i].frameBuffer, nullptr);
			m_ShadowMapPasses[i].frameBuffer = VK_NULL_HANDLE;
		}
	}

	for (int i = 0; i < m_CascadePasses.size(); i++)
	{
		for (int j = 0; j < SHADOW_MAP_CASCADE_COUNT; ++j) 
		{
			auto& frameBuf = m_CascadePasses[i].cascades[j].frameBuffer;
			if (frameBuf != VK_NULL_HANDLE) {
				vkDestroyFramebuffer(VulkanContext::GetDevice(), frameBuf, nullptr);
				frameBuf = VK_NULL_HANDLE;
			}
		}
	}

	for (int i = 0; i < m_OmniPasses.size(); i++)
	{
		for (int j = 0; j < OMNI_SHADOWMAPS_COUNT; ++j)
		{
			auto& frameBuf = m_OmniPasses[i].cubefaces[j].frameBuffer;
			if (frameBuf != VK_NULL_HANDLE) {
				vkDestroyFramebuffer(VulkanContext::GetDevice(), frameBuf, nullptr);
				frameBuf = VK_NULL_HANDLE;
			}
		}
	}

	if (m_ShadowMapRenderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(VulkanContext::GetDevice(), m_ShadowMapRenderPass, nullptr);
		m_ShadowMapRenderPass = VK_NULL_HANDLE;
	}

	if (m_ShadowMapPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(VulkanContext::GetDevice(), m_ShadowMapPipeline, nullptr);
		m_ShadowMapPipeline = VK_NULL_HANDLE;
	}

	for (Workspace& workspace : workspaces)
	{
		workspace.LightSpaces.Destroy();
		workspace.LightSpaces_Src.Destroy();
	}
	workspaces.clear();

	m_Allocator.Cleanup();

	if (m_ShadowMapPassPipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(VulkanContext::GetDevice(), m_ShadowMapPassPipelineLayout, nullptr);
		m_ShadowMapPassPipelineLayout = VK_NULL_HANDLE;
	}
}

// Set up a separate render pass for the offscreen frame buffer
void ShadowPipeline::CreateRenderPass()
{
	// resize offscreen passes
	const auto& shadowLights = SceneManager::Get()->getScene()->getShadowInstances();
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
	subpass.colorAttachmentCount = 0;													// No m_Color attachments
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

	// create shadow render pass
	VulkanContext::VK_CHECK(
		vkCreateRenderPass(VulkanContext::GetDevice(), &renderpassCreateInfo, nullptr, &m_ShadowMapRenderPass),
		"[Vulkan] Create Render pass failed"
	);

	uint32_t numShadowCasters[3] = { 0 };
	for (auto shadowLight : shadowLights)
		numShadowCasters[shadowLight->type]++;

	m_CascadePasses.resize(numShadowCasters[0]);
	m_OmniPasses.resize(numShadowCasters[1]);
	m_ShadowMapPasses.resize(numShadowCasters[2]);

	Cascade_CreateRenderPasses(numShadowCasters[0]);
	Omni_CreateRenderPasses(numShadowCasters[1]);
	ShadowMap_CreateRenderPasses(numShadowCasters[2]);
}

void ShadowPipeline::Rebuild()
{
	// TODO: do this shit bruh, but maybe not needed?
}

void ShadowPipeline::CreatePipeline()
{
	ShadowMap_CreateDescriptors();
	ShadowMap_CreatePipelineLayout();
	ShadowMap_CreateGraphicsPipeline();
}

void ShadowPipeline::Prepare(const Scene* scene, const CommandBuffer& commandBuffer)
{
	Workspace& workspace = workspaces[CURR_FRAME];

	const auto& shadowLights = SceneManager::Get()->getScene()->getShadowInstances();
	size_t needed_bytes = 0;
	for (int i = 0; i < shadowLights.size(); ++i)
	{
		switch (shadowLights[i]->type)
		{
		case 0:
			needed_bytes += 4 * sizeof(glm::mat4);
			break;
		case 1:
			needed_bytes += 6 * sizeof(glm::mat4);
			break;
		case 2:
			needed_bytes += 1 * sizeof(glm::mat4);
			break;
		}
	}

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
			.Write(workspace.set0_Lightspaces);

		NE_INFO("Reallocated SHADOWS to {} bytes", new_bytes);
	}

	assert(workspace.LightSpaces_Src.getSize() == workspace.LightSpaces.getSize());
	assert(workspace.LightSpaces_Src.getSize() >= needed_bytes);

	//copy LightSpaces into LightSpaces_Src:
	size_t offset = 0;
	{
		assert(workspace.LightSpaces_Src.data() != nullptr);

		for (int i = 0; i < shadowLights.size(); ++i) 
		{
			switch (shadowLights[i]->type)
			{
			case 0:
				memcpy(PTR_ADD(workspace.LightSpaces_Src.data(), offset), shadowLights[i]->m_Lightspaces.data(), SHADOW_MAP_CASCADE_COUNT * sizeof(glm::mat4));
				offset += SHADOW_MAP_CASCADE_COUNT * sizeof(glm::mat4);
				break;
			case 1:
				memcpy(PTR_ADD(workspace.LightSpaces_Src.data(), offset), shadowLights[i]->m_Lightspaces.data(), OMNI_SHADOWMAPS_COUNT * sizeof(glm::mat4));
				offset += OMNI_SHADOWMAPS_COUNT * sizeof(glm::mat4);
				break;
			case 2:
				memcpy(PTR_ADD(workspace.LightSpaces_Src.data(), offset), glm::value_ptr(shadowLights[i]->m_Lightspaces[0]), sizeof(glm::mat4));
				offset += sizeof(glm::mat4);
				break;
			}
		}
	}

	Buffer::CopyBuffer(commandBuffer, workspace.LightSpaces_Src.getBuffer(), workspace.LightSpaces.getBuffer(), offset);
}

void ShadowPipeline::Render(const Scene* scene, const CommandBuffer& commandBuffer)
{
	const auto& shadowLights = SceneManager::Get()->getScene()->getShadowInstances();

	// bind descriptors
	std::array< VkDescriptorSet, 2 > descriptor_sets{
		workspaces[CURR_FRAME].set0_Lightspaces,
		p_ObjectPipeline->workspaces[CURR_FRAME].set1_StorageBuffers // for transforms
	};

	vkCmdBindDescriptorSets(
		commandBuffer, //command buffer
		VK_PIPELINE_BIND_POINT_GRAPHICS, //pipeline bind point
		m_ShadowMapPassPipelineLayout, //pipeline layout
		0, //first set
		uint32_t(descriptor_sets.size()), descriptor_sets.data(), //descriptor sets count, ptr
		0, nullptr //dynamic offsets count, ptr
	);

	// bind and draw scene
	auto DrawOnce = [&](int lightIndex)
	{
		// bind pipeline
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ShadowMapPipeline);

		// push constant
		Push push{
			.lightspaceID = lightIndex
		};
		vkCmdPushConstants(commandBuffer, m_ShadowMapPassPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Push), &push);

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
	};
	
	// the following is a fucking messy indexing pile garbage, but its elegant in the way that it requires
	// no extra storage buffers, no extra pipelines, layouts, or descriptorsets.
	// everything is done on a single pipeline

	uint32_t passIndices[3] = {0}; // which pass are we executing
	int lightspaceMatrixID = 0; // offset of the lightspace matrix in the shader storage buffer
	// render each shadow map
	for (int lightIndex = 0; lightIndex < shadowLights.size(); ++lightIndex)
	{
		uint32_t type = shadowLights[lightIndex]->type;
		if (type == 0)
		{
			Cascade_SetViewports(commandBuffer, passIndices[type]);
			for (uint32_t cascadeIndex = 0; cascadeIndex < SHADOW_MAP_CASCADE_COUNT; ++cascadeIndex) 
			{
				Cascade_BeginRenderPass(commandBuffer, passIndices[type], cascadeIndex);
				DrawOnce(lightspaceMatrixID);
				lightspaceMatrixID++;
			}
		}
		else if(type == 1)
		{
			Omni_SetViewports(commandBuffer, passIndices[type]);
			for (uint32_t faceIndex = 0; faceIndex < OMNI_SHADOWMAPS_COUNT; ++faceIndex)
			{
				Omni_BeginRenderPass(commandBuffer, passIndices[type], faceIndex);
				DrawOnce(lightspaceMatrixID);
				lightspaceMatrixID++;
			}
		}
		else
		{
			ShadowMap_BeginRenderPass(commandBuffer, passIndices[type]);
			DrawOnce(lightspaceMatrixID);
			lightspaceMatrixID++;
		}
		passIndices[type]++;
	}
}

void ShadowPipeline::ShadowMap_CreateRenderPasses(uint32_t numPasses)
{
	// create frame buffers and image views for naive shadowpass (spotlights)
	for (uint32_t i = 0; i < numPasses; ++i)
	{
		m_ShadowMapPasses[i].width = SHADOWMAP_DIM;
		m_ShadowMapPasses[i].height = SHADOWMAP_DIM;

		m_ShadowMapPasses[i].depthAttachment = std::make_unique<ImageDepth>(glm::uvec2{ SHADOWMAP_DIM, SHADOWMAP_DIM }, VK_FORMAT_D16_UNORM);

		VkImageView view = m_ShadowMapPasses[i].depthAttachment->getView();

		VkFramebufferCreateInfo create_info
		{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = m_ShadowMapRenderPass,
			.attachmentCount = 1,
			.pAttachments = &view,
			.width = m_ShadowMapPasses[i].width,
			.height = m_ShadowMapPasses[i].height,
			.layers = 1,
		};

		VulkanContext::VK_CHECK(
			vkCreateFramebuffer(VulkanContext::GetDevice(), &create_info, nullptr, &m_ShadowMapPasses[i].frameBuffer),
			"[vulkan] Creating frame buffer failed"
		);
	}
}

void ShadowPipeline::ShadowMap_CreateDescriptors()
{
	workspaces.resize(VulkanContext::Get()->getFramesInFlight());

	for (Workspace& workspace : workspaces)
	{
		DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_Allocator)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.Build(workspace.set0_Lightspaces, set0_LightspacesLayout);
	}
}

void ShadowPipeline::ShadowMap_CreatePipelineLayout()
{
	std::array< VkDescriptorSetLayout, 2 > layouts{
		set0_LightspacesLayout,
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

	VulkanContext::VK_CHECK(vkCreatePipelineLayout(VulkanContext::GetDevice(), &create_info, nullptr, &m_ShadowMapPassPipelineLayout));
}

void ShadowPipeline::ShadowMap_CreateGraphicsPipeline()
{
	std::vector< VkDynamicState > dynamic_states{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_VERTEX_INPUT_EXT
	};

	VulkanGraphicsPipelineBuilder::Start()
		.SetDynamicStates(dynamic_states)
		.SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.SetRasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
		.Build("../spv/shaders/shadow/shadowmapping.vert.spv", "../spv/shaders/shadow/shadowmapping.frag.spv", &m_ShadowMapPipeline, m_ShadowMapPassPipelineLayout, m_ShadowMapRenderPass);
}

void ShadowPipeline::ShadowMap_BeginRenderPass(const CommandBuffer& commandBuffer, uint32_t passIndex)
{
	VkRenderPassBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = m_ShadowMapRenderPass,
		.framebuffer = m_ShadowMapPasses[passIndex].frameBuffer,
		.renderArea{
			.offset = {.x = 0, .y = 0},
			.extent = {.width = (uint32_t)m_ShadowMapPasses[passIndex].width, .height = (uint32_t)m_ShadowMapPasses[passIndex].height},
		},
		.clearValueCount = 1,
		.pClearValues = clear_values.data(),
	};

	vkCmdBeginRenderPass(commandBuffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

	VkRect2D scissor{
		.offset = {.x = 0, .y = 0},
		.extent = {.width = (uint32_t)m_ShadowMapPasses[passIndex].width, .height = (uint32_t)m_ShadowMapPasses[passIndex].height},
	};
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	VkViewport viewport{
		.x = 0.0f,
		.y = 0.0f,
		.width = float(m_ShadowMapPasses[passIndex].width),
		.height = float(m_ShadowMapPasses[passIndex].height),
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

void ShadowPipeline::Cascade_CreateRenderPasses(uint32_t numPasses)
{
	// create cascade passes
	for (uint32_t i = 0; i < numPasses; ++i)
	{
		m_CascadePasses[i].width = CASCADED_SHADOWMAP_DIM;
		m_CascadePasses[i].height = CASCADED_SHADOWMAP_DIM;

		for (uint32_t cascadeIndex = 0; cascadeIndex < SHADOW_MAP_CASCADE_COUNT; cascadeIndex++) 
		{

			m_CascadePasses[i].cascades[cascadeIndex].depthAttachment = std::make_unique<ImageDepth>(glm::uvec2{ CASCADED_SHADOWMAP_DIM, CASCADED_SHADOWMAP_DIM }, VK_FORMAT_D16_UNORM);
			VkImageView view = m_CascadePasses[i].cascades[cascadeIndex].depthAttachment->getView();

			VkFramebufferCreateInfo create_info
			{
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = m_ShadowMapRenderPass,
				.attachmentCount = 1,
				.pAttachments = &view,
				.width = m_CascadePasses[i].width,
				.height = m_CascadePasses[i].height,
				.layers = 1,
			};

			VulkanContext::VK_CHECK(
				vkCreateFramebuffer(VulkanContext::GetDevice(), &create_info, nullptr, &m_CascadePasses[i].cascades[cascadeIndex].frameBuffer),
				"[vulkan] Creating frame buffer failed"
			);
		}
	}
}

void ShadowPipeline::Cascade_SetViewports(const CommandBuffer& commandBuffer, uint32_t passIndex)
{
	VkRect2D scissor{
		.offset = {.x = 0, .y = 0},
		.extent = {.width = m_CascadePasses[passIndex].width, .height = m_CascadePasses[passIndex].height},
	};
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	VkViewport viewport{
		.x = 0.0f,
		.y = 0.0f,
		.width = float(m_CascadePasses[passIndex].width),
		.height = float(m_CascadePasses[passIndex].height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	vkCmdSetDepthBias(
		commandBuffer,
		depthBiasConstant,
		0.0f,
		depthBiasSlope);
}

void ShadowPipeline::Cascade_BeginRenderPass(const CommandBuffer& commandBuffer, uint32_t passIndex, uint32_t cascadeIndex)
{
	VkRenderPassBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = m_ShadowMapRenderPass,
		.framebuffer = m_CascadePasses[passIndex].cascades[cascadeIndex].frameBuffer,
		.renderArea{
			.offset = {.x = 0, .y = 0},
			.extent = {
				.width = m_CascadePasses[passIndex].width, 
				.height = m_CascadePasses[passIndex].height
			},
		},
		.clearValueCount = 1,
		.pClearValues = clear_values.data(),
	};

	vkCmdBeginRenderPass(commandBuffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void ShadowPipeline::Omni_CreateRenderPasses(uint32_t numPasses)
{
	// create cascade passes
	for (uint32_t i = 0; i < numPasses; ++i)
	{
		m_OmniPasses[i].width = SHADOWMAP_DIM;
		m_OmniPasses[i].height = SHADOWMAP_DIM;
		for (uint32_t faceIndex = 0; faceIndex < OMNI_SHADOWMAPS_COUNT; faceIndex++) {

			m_OmniPasses[i].cubefaces[faceIndex].depthAttachment = std::make_unique<ImageDepth>(glm::uvec2{ SHADOWMAP_DIM, SHADOWMAP_DIM }, VK_FORMAT_D16_UNORM);
			VkImageView view = m_OmniPasses[i].cubefaces[faceIndex].depthAttachment->getView();

			VkFramebufferCreateInfo create_info
			{
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = m_ShadowMapRenderPass,
				.attachmentCount = 1,
				.pAttachments = &view,
				.width = m_OmniPasses[i].width,
				.height = m_OmniPasses[i].height,
				.layers = 1,
			};

			VulkanContext::VK_CHECK(
				vkCreateFramebuffer(VulkanContext::GetDevice(), &create_info, nullptr, &m_OmniPasses[i].cubefaces[faceIndex].frameBuffer),
				"[vulkan] Creating frame buffer failed"
			);
		}
	}
}

void ShadowPipeline::Omni_SetViewports(const CommandBuffer& commandBuffer, uint32_t passIndex)
{
	VkRect2D scissor{
		.offset = {.x = 0, .y = 0},
		.extent = {.width = m_OmniPasses[passIndex].width, .height = m_OmniPasses[passIndex].height},
	};
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	VkViewport viewport{
		.x = 0.0f,
		.y = 0.0f,
		.width = float(m_OmniPasses[passIndex].width),
		.height = float(m_OmniPasses[passIndex].height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	vkCmdSetDepthBias(
		commandBuffer,
		depthBiasConstant,
		0.0f,
		depthBiasSlope);
}

void ShadowPipeline::Omni_BeginRenderPass(const CommandBuffer& commandBuffer, uint32_t passIndex, uint32_t faceIndex)
{
	VkRenderPassBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = m_ShadowMapRenderPass,
		.framebuffer = m_OmniPasses[passIndex].cubefaces[faceIndex].frameBuffer,
		.renderArea{
			.offset = {.x = 0, .y = 0},
			.extent = {
				.width = m_OmniPasses[passIndex].width,
				.height = m_OmniPasses[passIndex].height
			},
		},
		.clearValueCount = 1,
		.pClearValues = clear_values.data(),
	};

	vkCmdBeginRenderPass(commandBuffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}
