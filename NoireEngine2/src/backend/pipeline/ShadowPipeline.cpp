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

#if defined(__ANDROID__)
#define SHADOWMAP_DIM 512
#define CASCADED_SHADOWMAP_DIM 1024
#else
#define SHADOWMAP_DIM 1024
#define CASCADED_SHADOWMAP_DIM 2048
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
	for (Workspace& workspace : workspaces)
	{
		// wait for thread first
		workspace.threadPool->Wait();

		workspace.LightSpaces.Destroy();
		workspace.LightSpaces_Src.Destroy();

		workspace.secondaryCommandBuffers.clear();
	}
	workspaces.clear();

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
	CreateDescriptors();
	CreatePipelineLayout();
	CreateGraphicsPipeline();
	PrepareShadowRenderThreads();
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

void ShadowPipeline::Render(const Scene* scene, const CommandBuffer& primaryCmdBuffer)
{
	const auto& shadowLights = SceneManager::Get()->getScene()->getShadowInstances();

	// Inheritance info for the secondary command buffers
	VkCommandBufferInheritanceInfo inheritanceInfo{};
	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.renderPass = m_ShadowMapRenderPass;

	// the following is a fucking messy indexing pile garbage, but its elegant in the way that it requires
	// no extra storage buffers, no extra pipelines, layouts, or descriptorsets.
	// everything is done on a single pipeline

	Workspace& workspace = workspaces[CURR_FRAME];

	// record into secondary command buffers in a separate thread
	uint32_t passIndices[3] = {0}; // which pass are we executing
	int lightspaceMatrixID = 0; // offset of the lightspace matrix in the shader storage buffer
	// render each shadow map
	for (int lightIndex = 0; lightIndex < shadowLights.size(); ++lightIndex)
	{
		uint32_t type = shadowLights[lightIndex]->type;
		switch (type) 
		{
		case 0:
			for (uint32_t cascadeIndex = 0; cascadeIndex < SHADOW_MAP_CASCADE_COUNT; ++cascadeIndex)
			{
				inheritanceInfo.framebuffer = m_CascadePasses[passIndices[type]].cascades[cascadeIndex].frameBuffer;

				workspace.threadPool->threads[lightspaceMatrixID]->addJob([=] {
					T_RenderShadows(lightspaceMatrixID, inheritanceInfo, scene, type, CASCADED_SHADOWMAP_DIM, CASCADED_SHADOWMAP_DIM);
				});

				lightspaceMatrixID++;
			}
			break;
		case 1:
			for (uint32_t faceIndex = 0; faceIndex < OMNI_SHADOWMAPS_COUNT; ++faceIndex)
			{
				inheritanceInfo.framebuffer = m_OmniPasses[passIndices[type]].cubefaces[faceIndex].frameBuffer;

				workspace.threadPool->threads[lightspaceMatrixID]->addJob([=] {
					T_RenderShadows(lightspaceMatrixID, inheritanceInfo, scene, type, SHADOWMAP_DIM, SHADOWMAP_DIM);
				});
				lightspaceMatrixID++;
			}
			break;
		case 2:
			inheritanceInfo.framebuffer = m_ShadowMapPasses[passIndices[type]].frameBuffer;

			workspace.threadPool->threads[lightspaceMatrixID]->addJob([=] {
				T_RenderShadows(lightspaceMatrixID, inheritanceInfo, scene, type, SHADOWMAP_DIM, SHADOWMAP_DIM);
			});
			lightspaceMatrixID++;
			break;
		}
		passIndices[type]++;
	}

	// join threads
	workspace.threadPool->Wait();

	for (int i = 0; i < 3; i++)
		passIndices[i] = 0;
	lightspaceMatrixID = 0;

	// render each shadow map in the main thread by calling a bunch of executes
	// on the primary command buffer
	for (int lightIndex = 0; lightIndex < shadowLights.size(); ++lightIndex)
	{
		uint32_t type = shadowLights[lightIndex]->type;
		switch (type)
		{
		case 0:
			for (uint32_t cascadeIndex = 0; cascadeIndex < SHADOW_MAP_CASCADE_COUNT; ++cascadeIndex)
			{
				BeginRenderPass(primaryCmdBuffer, m_CascadePasses[passIndices[type]].cascades[cascadeIndex].frameBuffer, CASCADED_SHADOWMAP_DIM, CASCADED_SHADOWMAP_DIM);
				vkCmdExecuteCommands(primaryCmdBuffer, 1, &workspace.secondaryCommandBuffers[lightspaceMatrixID]->getCommandBuffer());
				vkCmdEndRenderPass(primaryCmdBuffer);
				lightspaceMatrixID++;
			}
			break;
		case 1:
			for (uint32_t faceIndex = 0; faceIndex < OMNI_SHADOWMAPS_COUNT; ++faceIndex)
			{
				BeginRenderPass(primaryCmdBuffer, m_OmniPasses[passIndices[type]].cubefaces[faceIndex].frameBuffer, SHADOWMAP_DIM, SHADOWMAP_DIM);
				vkCmdExecuteCommands(primaryCmdBuffer, 1, &workspace.secondaryCommandBuffers[lightspaceMatrixID]->getCommandBuffer());
				vkCmdEndRenderPass(primaryCmdBuffer);
				lightspaceMatrixID++;
			}
			break;
		case 2:
			BeginRenderPass(primaryCmdBuffer, m_ShadowMapPasses[passIndices[type]].frameBuffer, SHADOWMAP_DIM, SHADOWMAP_DIM);
			vkCmdExecuteCommands(primaryCmdBuffer, 1, &workspace.secondaryCommandBuffers[lightspaceMatrixID]->getCommandBuffer());
			vkCmdEndRenderPass(primaryCmdBuffer);
			lightspaceMatrixID++;
			break;
		}
		passIndices[type]++;
	}
}

void ShadowPipeline::PrepareShadowRenderThreads()
{
	uint32_t threads = 0;
	const auto& shadowLights = SceneManager::Get()->getScene()->getShadowInstances();
	for (int lightIndex = 0; lightIndex < shadowLights.size(); ++lightIndex)
	{
		switch (shadowLights[lightIndex]->type)
		{
		case 0:
			threads += SHADOW_MAP_CASCADE_COUNT;
			break;
		case 1:
			threads += OMNI_SHADOWMAPS_COUNT;
			break;
		case 2:
			threads++;
			break;
		}
	}

	for (auto& workspace : workspaces)
	{
		workspace.threadPool = std::make_unique<ThreadPool>();
		workspace.threadPool->SetThreadCount(threads);
		workspace.secondaryCommandBuffers.resize(threads);
	}
	// initialization will be done on separate threads
	NE_INFO("Found {} shadow threads.", threads);
}

void ShadowPipeline::T_RenderShadows(uint32_t tid, VkCommandBufferInheritanceInfo inheritance, const Scene* scene, uint32_t lightType, uint32_t width, uint32_t height)
{
	Workspace& workspace = workspaces[CURR_FRAME];

	if (!workspace.secondaryCommandBuffers[tid]) {
		workspace.secondaryCommandBuffers[tid] = std::make_unique<CommandBuffer>(false, VK_QUEUE_GRAPHICS_BIT, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
	}

	workspace.secondaryCommandBuffers[tid]->Begin(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &inheritance);

	// bind descriptors
	std::array< VkDescriptorSet, 2 > descriptor_sets{
		workspaces[CURR_FRAME].set0_Lightspaces,
		p_ObjectPipeline->workspaces[CURR_FRAME].set1_StorageBuffers // for transforms
	};

	vkCmdBindDescriptorSets(
		workspace.secondaryCommandBuffers[tid]->getCommandBuffer(), //command buffer
		VK_PIPELINE_BIND_POINT_GRAPHICS, //pipeline bind point
		m_ShadowMapPassPipelineLayout, //pipeline layout
		0, //first set
		uint32_t(descriptor_sets.size()), descriptor_sets.data(), //descriptor sets count, ptr
		0, nullptr //dynamic offsets count, ptr
	);

	// bind pipeline
	vkCmdBindPipeline(workspace.secondaryCommandBuffers[tid]->getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_ShadowMapPipeline);

	SetViewports(*workspace.secondaryCommandBuffers[tid].get(), width, height);

	const auto& allInstances = scene->getObjectInstances();
	const auto& indirectBatches = p_ObjectPipeline->getIndirectBatches();

	Push push{ .lightspaceID = (int)tid };
	vkCmdPushConstants(workspace.secondaryCommandBuffers[tid]->getCommandBuffer(), m_ShadowMapPassPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Push), &push);

	//draw all instances in relation to a certain material:
	uint32_t offsetIndex = 0;
	VertexInput* previouslyBindedVertex = nullptr;

	for (int workflowIndex = 0; workflowIndex < allInstances.size(); ++workflowIndex)
	{
		const auto& workflowInstances = allInstances[workflowIndex];
		if (workflowInstances.empty())
			continue;

		uint32_t instanceCount = (uint32_t)workflowInstances.size();

		// draw each batch
		for (const ObjectPipeline::IndirectBatch& draw : indirectBatches[workflowIndex])
		{
			VertexInput* vertexInputPtr = draw.mesh->getVertexInput();
			if (vertexInputPtr != previouslyBindedVertex) {
				vertexInputPtr->Bind(*workspace.secondaryCommandBuffers[tid].get());
				previouslyBindedVertex = vertexInputPtr;
			}

			draw.mesh->Bind(*workspace.secondaryCommandBuffers[tid].get());

			constexpr uint32_t stride = sizeof(VkDrawIndexedIndirectCommand);
			VkDeviceSize offset = (draw.firstInstanceIndex + offsetIndex) * stride;

			vkCmdDrawIndexedIndirect(workspace.secondaryCommandBuffers[tid]->getCommandBuffer(), VulkanContext::Get()->getIndirectBuffer()->getBuffer(), offset, draw.count, stride);
			ObjectPipeline::NumDrawCalls++;
		}
		offsetIndex += instanceCount;
	}

	workspace.secondaryCommandBuffers[tid]->End();
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

void ShadowPipeline::CreateDescriptors()
{
	workspaces.resize(VulkanContext::Get()->getFramesInFlight());

	for (Workspace& workspace : workspaces)
	{
		DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_Allocator)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.Build(workspace.set0_Lightspaces, set0_LightspacesLayout);
	}
}

void ShadowPipeline::CreatePipelineLayout()
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

void ShadowPipeline::CreateGraphicsPipeline()
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

void ShadowPipeline::SetViewports(const CommandBuffer& commandBuffer, uint32_t width, uint32_t height)
{
	VkRect2D scissor{
	.offset = {.x = 0, .y = 0},
	.extent = {.width = width, .height = height},
	};
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	VkViewport viewport{
		.x = 0.0f,
		.y = 0.0f,
		.width = float(width),
		.height = float(height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
}

void ShadowPipeline::BeginRenderPass(const CommandBuffer& commandBuffer, VkFramebuffer frameBuffer, uint32_t width, uint32_t height)
{
	VkRenderPassBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = m_ShadowMapRenderPass,
		.framebuffer = frameBuffer,
		.renderArea{
			.offset = {.x = 0, .y = 0},
			.extent = {.width = width, .height = height},
		},
		.clearValueCount = 1,
		.pClearValues = clear_values.data(),
	};

	vkCmdBeginRenderPass(commandBuffer, &begin_info, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
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