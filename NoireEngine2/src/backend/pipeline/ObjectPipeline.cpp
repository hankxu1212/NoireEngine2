#include "ObjectPipeline.hpp"

#include <array>
#include <algorithm>
#include <execution>

#include "backend/VulkanContext.hpp"
#include "backend/shader/VulkanShader.h"
#include "math/Math.hpp"
#include "core/Time.hpp"
#include "core/Timer.hpp"
#include "renderer/object/Mesh.hpp"
#include "renderer/materials/Material.hpp"
#include "core/resources/Files.hpp"
#include "renderer/scene/Scene.hpp"
#include "utils/Logger.hpp"

#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, perspective 

static uint32_t vert_code[] =
#include "spv/shaders/objects.vert.inl"
;

static uint32_t frag_code[] =
#include "spv/shaders/objects.frag.inl"
;

ObjectPipeline::ObjectPipeline()
{
}

ObjectPipeline::~ObjectPipeline()
{
	VkDevice device = VulkanContext::GetDevice();

	textures.clear();

	for (Workspace& workspace : workspaces) 
	{
		workspace.Transforms_src.Destroy();
		workspace.Transforms.Destroy();
		workspace.World.Destroy();
		workspace.World_src.Destroy();
	}
	workspaces.clear();

	m_DescriptorAllocator.Cleanup();

	if (m_PipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
		m_PipelineLayout = VK_NULL_HANDLE;
	}

	if (m_Pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(device, m_Pipeline, nullptr);
		m_Pipeline = VK_NULL_HANDLE;
	}

	if (set0_World != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(device, set0_World, nullptr);
		set0_World = VK_NULL_HANDLE;
	}

	if (set1_Transforms != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(device, set1_Transforms, nullptr);
		set1_Transforms = VK_NULL_HANDLE;
	}

	if (set2_TEXTURE != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(device, set2_TEXTURE, nullptr);
		set2_TEXTURE = VK_NULL_HANDLE;
	}

	DestroyFrameBuffers();

	if (m_Renderpass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(VulkanContext::GetDevice(), m_Renderpass, nullptr);
	}
}

void ObjectPipeline::CreateRenderPass()
{
	std::array< VkAttachmentDescription, 2 > attachments{
		VkAttachmentDescription { //0 - color attachment:
			.format = VulkanContext::Get()->getSurface(0)->getFormat().format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		},
		VkAttachmentDescription{ //1 - depth attachment:
			.format = VK_FORMAT_D32_SFLOAT_S8_UINT,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		},
	};

	VkAttachmentReference color_attachment_ref{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentReference depth_attachment_ref{
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpass{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment_ref,
		.pDepthStencilAttachment = &depth_attachment_ref,
	};

	//this defers the image load actions for the attachments:
	std::array< VkSubpassDependency, 2 > dependencies{
		VkSubpassDependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		},
		VkSubpassDependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		}
	};

	VkRenderPassCreateInfo create_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = uint32_t(attachments.size()),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = uint32_t(dependencies.size()),
		.pDependencies = dependencies.data(),
	};

	VulkanContext::VK_CHECK(
		vkCreateRenderPass(VulkanContext::GetDevice(), &create_info, nullptr, &m_Renderpass),
		"[Vulkan] Create Render pass failed"
	);
}

void ObjectPipeline::CreatePipelineLayouts()
{
	///////////////////////////////////////////////////////////////////////
	// Shaders

	VulkanShader vertModule(vert_code, VulkanShader::ShaderStage::Vertex);
	VulkanShader fragModule(frag_code, VulkanShader::ShaderStage::Frag);

	std::array< VkPipelineShaderStageCreateInfo, 2 > stages{
		vertModule.shaderStage(),
		fragModule.shaderStage()
	};

	///////////////////////////////////////////////////////////////////////

	std::vector< VkDynamicState > dynamic_states{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_VERTEX_INPUT_EXT
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

	//this pipeline will draw triangles:
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
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
		.lineWidth = 1.0f,
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
		.pVertexInputState = /*&VertexInput::array_input_state*/VK_NULL_HANDLE,
		.pInputAssemblyState = &input_assembly_state,
		.pViewportState = &viewport_state,
		.pRasterizationState = &rasterization_state,
		.pMultisampleState = &multisample_state,
		.pDepthStencilState = &depth_stencil_state,
		.pColorBlendState = &color_blend_state,
		.pDynamicState = &dynamic_state,
		.layout = m_PipelineLayout,
		.renderPass = m_Renderpass,
		.subpass = 0,
	};

	VulkanContext::VK_CHECK(
		vkCreateGraphicsPipelines(VulkanContext::GetDevice(), VK_NULL_HANDLE, 1, &create_info, nullptr, &m_Pipeline),
		"[Vulkan] Create pipeline failed");
}

void ObjectPipeline::CreateDescriptors()
{
	workspaces.resize(VulkanContext::Get()->getWorkspaceSize());

	// create world and transform descriptor
	for (Workspace& workspace : workspaces)
	{
		workspace.World_src = Buffer(
			sizeof(Scene::SceneUniform),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			Buffer::Mapped
		);
		workspace.World = Buffer(
			sizeof(Scene::SceneUniform),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		VkDescriptorBufferInfo World_info
		{
			.buffer = workspace.World.getBuffer(),
			.offset = 0,
			.range = workspace.World.getSize(),
		};

		DescriptorBuilder builder;
		builder.Start(&m_DescriptorLayoutCache, &m_DescriptorAllocator)
			.BindBuffer(0, &World_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.Build(workspace.World_descriptors, set0_World);

		VkDescriptorBufferInfo Transforms_info = CreateTransformStorageBuffer(workspace, 4096);
		DescriptorBuilder builder2;
		builder2.Start(&m_DescriptorLayoutCache, &m_DescriptorAllocator)
			.BindBuffer(0, &Transforms_info, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.Build(workspace.Transforms_descriptors, set1_Transforms);
	}

	// create texture descriptor
	textures.resize(1);
	textures[0] = std::make_shared<Image2D>(Files::Path("../textures/default_gray.png"));
	texture_descriptors.resize(1);
	for (auto& tex : textures)
	{
		VkDescriptorImageInfo texInfo
		{
			.sampler = tex->getSampler(),
			.imageView = tex->getView(),
			.imageLayout = tex->getLayout(),
		};

		DescriptorBuilder builder;
		builder.Start(&m_DescriptorLayoutCache, &m_DescriptorAllocator)
			.BindImage(0, &texInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.Build(texture_descriptors[0], set2_TEXTURE);
	}

	std::array< VkDescriptorSetLayout, 3 > layouts{
		set0_World,
		set1_Transforms,
		set2_TEXTURE,
	};

	VkPushConstantRange range{
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0,
		.size = sizeof(MaterialPush),
	};

	VkPipelineLayoutCreateInfo create_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = uint32_t(layouts.size()),
		.pSetLayouts = layouts.data(),
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &range,
	};

	VulkanContext::VK_CHECK(vkCreatePipelineLayout(VulkanContext::GetDevice(), &create_info, nullptr, &m_PipelineLayout),
		"[Vulkan] Create pipeline layout failed.");
}

void ObjectPipeline::Rebuild()
{
	if (s_SwapchainDepthImage != nullptr && s_SwapchainDepthImage->getImage() != VK_NULL_HANDLE) {
		DestroyFrameBuffers();
	}

	// TODO: add support for multiple swapchains
	const SwapChain* swapchain = VulkanContext::Get()->getSwapChain(0);
	s_SwapchainDepthImage = std::make_unique<ImageDepth>(swapchain->getExtentVec2(), VK_SAMPLE_COUNT_1_BIT);

	//Make framebuffers for each swapchain image:
	m_Framebuffers.assign(swapchain->getImageViews().size(), VK_NULL_HANDLE);
	for (size_t i = 0; i < swapchain->getImageViews().size(); ++i) {
		std::array< VkImageView, 2 > attachments{
			swapchain->getImageViews()[i],
			s_SwapchainDepthImage->getView()
		};
		VkFramebufferCreateInfo create_info{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = m_Renderpass,
			.attachmentCount = uint32_t(attachments.size()),
			.pAttachments = attachments.data(),
			.width = swapchain->getExtent().width,
			.height = swapchain->getExtent().height,
			.layers = 1,
		};

		VulkanContext::VK_CHECK(
			vkCreateFramebuffer(VulkanContext::GetDevice(), &create_info, nullptr, &m_Framebuffers[i]),
			"[vulkan] Creating frame buffer failed"
		);
	}
}

void ObjectPipeline::CreatePipeline()
{
	CreateDescriptors();
	CreatePipelineLayouts();
}

void ObjectPipeline::Render(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId)
{
	PushSceneDrawInfo(scene, commandBuffer, surfaceId);

	VkExtent2D swapChainExtent = VulkanContext::Get()->getSwapChain()->getExtent();

	static std::array< VkClearValue, 2 > clear_values{
		VkClearValue{.color{.float32{0.2f, 0.2f, 0.2f, 0.2f} } },
		VkClearValue{.depthStencil{.depth = 1.0f, .stencil = 0 } },
	};

	VkRenderPassBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = m_Renderpass,
		.framebuffer = m_Framebuffers[VulkanContext::Get()->getCurrentFrame()],
		.renderArea{
			.offset = {.x = 0, .y = 0},
			.extent = swapChainExtent,
		},
		.clearValueCount = uint32_t(clear_values.size()),
		.pClearValues = clear_values.data(),
	};

	vkCmdBeginRenderPass(commandBuffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
	{
		VkRect2D scissor{
			.offset = {.x = 0, .y = 0},
			.extent = swapChainExtent,
		};
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
			
		VkViewport viewport{
			.x = 0.0f,
			.y = 0.0f,
			.width = float(swapChainExtent.width),
			.height = float(swapChainExtent.height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		RenderPass(scene, commandBuffer, surfaceId);
	}
	vkCmdEndRenderPass(commandBuffer);
}

void ObjectPipeline::Update(const Scene* scene)
{

}

void ObjectPipeline::PushSceneDrawInfo(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId)
{
	Workspace& workspace = workspaces[surfaceId];

	//upload world info:
	{ 
		//host-side copy into World_src:
		memcpy(workspace.World_src.data(), scene->getSceneUniformPtr(), scene->getSceneUniformSize());

		//add device-side copy from World_src -> World:
		assert(workspace.World_src.getSize() == workspace.World.getSize());
		Buffer::CopyBuffer(commandBuffer, workspace.World_src.getBuffer(), workspace.World.getBuffer(), workspace.World_src.getSize());
	}

	const std::vector<ObjectInstance>& sceneObjectInstances = scene->getObjectInstances();

	//upload object transforms and allocate needed bytes
	if (!sceneObjectInstances.empty()) 
	{
		size_t needed_bytes = sceneObjectInstances.size() * sizeof(ObjectInstance::TransformUniform);
		if (workspace.Transforms_src.getBuffer() == VK_NULL_HANDLE 
			|| workspace.Transforms_src.getSize() < needed_bytes) 
		{
			//round to next multiple of 4k to avoid re-allocating continuously if vertex count grows slowly:
			size_t new_bytes = ((needed_bytes + 4096) / 4096) * 4096;

			workspace.Transforms_src.Destroy();
			workspace.Transforms.Destroy();

			//update the descriptor set:
			VkDescriptorBufferInfo Transforms_info = CreateTransformStorageBuffer(workspace, new_bytes);

			DescriptorBuilder builder;
			builder.Start(&m_DescriptorLayoutCache, &m_DescriptorAllocator)
				.BindBuffer(0, &Transforms_info, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
				.Write(workspace.Transforms_descriptors);
		}

		assert(workspace.Transforms_src.getSize() == workspace.Transforms.getSize());
		assert(workspace.Transforms_src.getSize() >= needed_bytes);

		{ //copy transforms into Transforms_src:
			assert(workspace.Transforms_src.data() != nullptr);
			ObjectInstance::TransformUniform* out = reinterpret_cast<ObjectInstance::TransformUniform*>(workspace.Transforms_src.data());
			for (ObjectInstance const& inst : sceneObjectInstances) {
				*out = inst.m_TransformUniform;
				++out;
			}
		}

		Buffer::CopyBuffer(commandBuffer, workspace.Transforms_src.getBuffer(), workspace.Transforms.getBuffer(), needed_bytes);

		{ //memory barrier to make sure copies complete before rendering happens:
			std::array< VkBufferMemoryBarrier, 1> memoryBarriers = { 
				workspace.Transforms.CreateMemoryBarrier(VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT),
			};

			// we only allow a workspace to be "reused" once its work has been fully cleared and executed 
			// so src is never in danger of being overwritten 
			// -- we only write to it by direct memory mapped writes from the host and the device reads it with the copy command
			// -- Ajax

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, //srcStageMask
				VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, //dstStageMask
				0, //dependencyFlags
				0, nullptr, //memoryBarriers (count, data)
				1, memoryBarriers.data(), //bufferMemoryBarriers (count, data)
				0, nullptr //imageMemoryBarriers (count, data)
			);
		}
	}
}

VkDescriptorBufferInfo ObjectPipeline::CreateTransformStorageBuffer(Workspace& workspace, size_t new_bytes)
{
	workspace.Transforms_src = Buffer(
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
	std::cout << "Re-allocated object transforms buffers to " << new_bytes << " bytes." << std::endl;
	return Transforms_info;
}

//draw with the objects pipeline:
void ObjectPipeline::RenderPass(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

	Workspace& workspace = workspaces[surfaceId];
	{ //bind Transforms descriptor set:
		std::array< VkDescriptorSet, 2 > descriptor_sets{
			workspace.World_descriptors, //0: World
			workspace.Transforms_descriptors, //1: Transforms
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

	//bind texture descriptor set: (temporary, dont look)
	vkCmdBindDescriptorSets(
		commandBuffer, //command buffer
		VK_PIPELINE_BIND_POINT_GRAPHICS, //pipeline bind point
		m_PipelineLayout, //pipeline layout
		2, //second set
		1, &texture_descriptors[0], //descriptor sets count, ptr
		0, nullptr //dynamic offsets count, ptr
	);

	//draw all instances:
	const std::vector<ObjectInstance>& sceneObjectInstances = scene->getObjectInstances();
	ObjectsDrawn = sceneObjectInstances.size();
	std::vector<IndirectBatch> draws = CompactDraws(sceneObjectInstances);
	NumDrawCalls = draws.size();

	//encode the draw data of each object into the indirect draw buffer
	VkDrawIndexedIndirectCommand* drawCommands = (VkDrawIndexedIndirectCommand*)VulkanContext::Get()->getIndirectBuffer()->data();

	VerticesDrawn = 0;
	for (auto i = 0; i < ObjectsDrawn; i++)
	{
		drawCommands[i].indexCount = sceneObjectInstances[i].mesh->getIndexCount();
		drawCommands[i].instanceCount = 1;
		drawCommands[i].firstIndex = 0;
		drawCommands[i].vertexOffset = 0;
		drawCommands[i].firstInstance = i;

		VerticesDrawn += sceneObjectInstances[i].mesh->getVertexCount();
	}

	VertexInput* previouslyBindedVertex = nullptr;
	for (IndirectBatch& draw : draws)
	{
		VertexInput* vertexInputPtr = draw.mesh->getVertexInput();
		if (vertexInputPtr != previouslyBindedVertex) {
			vertexInputPtr->Bind(commandBuffer);
			previouslyBindedVertex = vertexInputPtr;
		}

		draw.mesh->Bind(commandBuffer);
		draw.material->Push(commandBuffer, m_PipelineLayout);

		VkDeviceSize offset = draw.first * sizeof(VkDrawIndexedIndirectCommand);
		uint32_t stride = sizeof(VkDrawIndexedIndirectCommand);

		vkCmdDrawIndexedIndirect(commandBuffer, VulkanContext::Get()->getIndirectBuffer()->getBuffer(), offset, draw.count, stride);
	}
}

std::vector<ObjectPipeline::IndirectBatch> ObjectPipeline::CompactDraws(const std::vector<ObjectInstance>& objects)
{
	std::vector<IndirectBatch> draws;

	draws.emplace_back(objects[0].mesh, objects[0].material, 0, 1);

	for (uint32_t i = 1; i < objects.size(); i++)
	{
		//compare the mesh and material with the end of the vector of draws
		bool sameMesh = objects[i].mesh == draws.back().mesh;
		bool sameMaterial = objects[i].material == draws.back().material;

		if (sameMesh && sameMaterial)
			draws.back().count++;
		else
			draws.emplace_back(objects[i].mesh, objects[i].material, i, 1);
	}
	return draws;
}


void ObjectPipeline::DestroyFrameBuffers()
{
	for (VkFramebuffer& framebuffer : m_Framebuffers)
	{
		assert(framebuffer != VK_NULL_HANDLE);
		vkDestroyFramebuffer(VulkanContext::GetDevice(), framebuffer, nullptr);
		framebuffer = VK_NULL_HANDLE;
	}
	m_Framebuffers.clear();
	s_SwapchainDepthImage.reset();
}