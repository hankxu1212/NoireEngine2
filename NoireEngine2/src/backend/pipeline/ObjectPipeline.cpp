#include "ObjectPipeline.hpp"

#include <array>

#include "backend/VulkanContext.hpp"
#include "backend/shader/VulkanShader.h"
#include "math/Math.hpp"
#include "core/Time.hpp"

#include "renderer/scene/Scene.hpp"

#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, perspective 

#include "core/resources/Files.hpp"

static uint32_t vert_code[] =
#include "spv/shaders/objects.vert.inl"
;

static uint32_t frag_code[] =
#include "spv/shaders/objects.frag.inl"
;

ObjectPipeline::ObjectPipeline(Renderer* renderer) :
	VulkanPipeline(renderer)
{
	CreateDescriptors();

	CreateDescriptorPool();

	CreatePipeline(m_BindedRenderer->m_Renderpass, 0);

	PrepareWorkspace();
}

ObjectPipeline::~ObjectPipeline()
{
	VkDevice device = VulkanContext::GetDevice();

	if (texture_descriptor_pool) 
	{
		vkDestroyDescriptorPool(device, texture_descriptor_pool, nullptr);
		texture_descriptor_pool = nullptr;

		//this also frees the descriptor sets allocated from the pool:
		texture_descriptors.clear();
	}

	textures.clear();

	for (Workspace& workspace : workspaces) 
	{
		workspace.Transforms_src.Destroy();
		workspace.Transforms.Destroy();
		workspace.World.Destroy();
		workspace.World_src.Destroy();
	}
	workspaces.clear();

	if (m_DescriptorPool) {
		vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
		m_DescriptorPool = nullptr;
		//(this also frees the descriptor sets allocated from the pool)
	}

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
}

void ObjectPipeline::CreateDescriptors()
{
	{ //the set0_World layout holds world info in a uniform buffer used in the fragment shader:
		std::array< VkDescriptorSetLayoutBinding, 1 > bindings{
			VkDescriptorSetLayoutBinding{
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
			},
		};

		VkDescriptorSetLayoutCreateInfo create_info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = uint32_t(bindings.size()),
			.pBindings = bindings.data(),
		};

		VulkanContext::VK_CHECK(vkCreateDescriptorSetLayout(VulkanContext::GetDevice(), &create_info, nullptr, &set0_World),
			"[vulkan] Create descriptor set layout failed");
	}

	{ //the set1_Transforms layout holds an array of Transform structures in a storage buffer used in the vertex shader:
		std::array< VkDescriptorSetLayoutBinding, 1 > bindings{
			VkDescriptorSetLayoutBinding{
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT
			},
		};

		VkDescriptorSetLayoutCreateInfo create_info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = uint32_t(bindings.size()),
			.pBindings = bindings.data(),
		};

		VulkanContext::VK_CHECK(vkCreateDescriptorSetLayout(VulkanContext::GetDevice(), &create_info, nullptr, &set1_Transforms),
			"[vulkan] Create descriptor set layout failed");
	}

	{ //the set2_TEXTURE layout has a single descriptor for a sampler2D used in the fragment shader:
		std::array< VkDescriptorSetLayoutBinding, 1 > bindings{
			VkDescriptorSetLayoutBinding{
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
			},
		};

		VkDescriptorSetLayoutCreateInfo create_info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = uint32_t(bindings.size()),
			.pBindings = bindings.data(),
		};

		VulkanContext::VK_CHECK(vkCreateDescriptorSetLayout(VulkanContext::GetDevice(), &create_info, nullptr, &set2_TEXTURE),
			"[vulkan] Create descriptor set layout failed");
	}

	std::array< VkDescriptorSetLayout, 3 > layouts{
		set0_World,
		set1_Transforms,
		set2_TEXTURE,
	};

	VkPipelineLayoutCreateInfo create_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = uint32_t(layouts.size()),
		.pSetLayouts = layouts.data(),
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr,
	};

	VulkanContext::VK_CHECK(vkCreatePipelineLayout(VulkanContext::GetDevice(), &create_info, nullptr, &m_PipelineLayout),
		"[Vulkan] Create pipeline layout failed.");
}

void ObjectPipeline::CreateDescriptorPool()
{
	uint32_t numWorkspace = VulkanContext::Get()->getWorkspaceSize(); //for easier-to-read counting

	//we only need uniform buffer descriptors for the moment:
	std::array< VkDescriptorPoolSize, 1> pool_sizes{
		VkDescriptorPoolSize{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 2 * numWorkspace, //one descriptor per set, one set per workspace
		},
	};

	VkDescriptorPoolCreateInfo create_info
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = 0, //because CREATE_FREE_DESCRIPTOR_SET_BIT isn't included, *can't* free individual descriptors allocated from this pool
		.maxSets = 3 * numWorkspace, //two sets per workspace
		.poolSizeCount = uint32_t(pool_sizes.size()),
		.pPoolSizes = pool_sizes.data(),
	};

	VulkanContext::VK_CHECK(vkCreateDescriptorPool(VulkanContext::GetDevice(), &create_info, nullptr, &m_DescriptorPool),
		"[Vulkan] Create descriptor pool failed");
}

void ObjectPipeline::CreatePipeline(VkRenderPass& renderpass, uint32_t subpass)
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
		.pVertexInputState = /*&Vertex::array_input_state*/VK_NULL_HANDLE,
		.pInputAssemblyState = &input_assembly_state,
		.pViewportState = &viewport_state,
		.pRasterizationState = &rasterization_state,
		.pMultisampleState = &multisample_state,
		.pDepthStencilState = &depth_stencil_state,
		.pColorBlendState = &color_blend_state,
		.pDynamicState = &dynamic_state,
		.layout = m_PipelineLayout,
		.renderPass = renderpass,
		.subpass = subpass,
	};

	VulkanContext::VK_CHECK(
		vkCreateGraphicsPipelines(VulkanContext::GetDevice(), VK_NULL_HANDLE, 1, &create_info, nullptr, &m_Pipeline),
		"[Vulkan] Create pipeline failed");
}

void ObjectPipeline::Render(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId)
{
	PushSceneDrawInfo(scene, commandBuffer, surfaceId);

	VkExtent2D swapChainExtent = VulkanContext::Get()->getSwapChain()->getExtent();

	static std::array< VkClearValue, 2 > clear_values{
		VkClearValue{.color{.float32{1.0f, 0.5f, 0.5f, 1.0f} } },
		VkClearValue{.depthStencil{.depth = 1.0f, .stencil = 0 } },
	};

	VkRenderPassBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = m_BindedRenderer->m_Renderpass,
		.framebuffer = m_BindedRenderer->m_Framebuffers[VulkanContext::Get()->getCurrentFrame()],
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

void ObjectPipeline::PrepareWorkspace()
{
	workspaces.resize(VulkanContext::Get()->getWorkspaceSize());
	std::cout << "Created workspace of size " << workspaces.size() << '\n';

	assert(workspaces.size() > 0);
	
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

		{ //allocate descriptor set for World descriptor
			VkDescriptorSetAllocateInfo alloc_info{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.descriptorPool = m_DescriptorPool,
				.descriptorSetCount = 1,
				.pSetLayouts = &set0_World,
			};

			VulkanContext::VK_CHECK(vkAllocateDescriptorSets(VulkanContext::GetDevice(), &alloc_info, &workspace.World_descriptors),
				"[vulkan] create descriptor set failed");
		}

		{ //allocate descriptor set for Transforms descriptor
			VkDescriptorSetAllocateInfo alloc_info{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.descriptorPool = m_DescriptorPool,
				.descriptorSetCount = 1,
				.pSetLayouts = &set1_Transforms,
			};

			VulkanContext::VK_CHECK(vkAllocateDescriptorSets(VulkanContext::GetDevice(), &alloc_info, &workspace.Transforms_descriptors),
				"[vulkan] create descriptor set failed");
		}

		{ 
			VkDescriptorBufferInfo World_info
			{
				.buffer = workspace.World.getBuffer(),
				.offset = 0,
				.range = workspace.World.getSize(),
			};

			std::array< VkWriteDescriptorSet, 1 > writes
			{
				VkWriteDescriptorSet{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = workspace.World_descriptors,
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.pBufferInfo = &World_info,
				},
			};

			vkUpdateDescriptorSets(
				VulkanContext::GetDevice(), //device
				uint32_t(writes.size()), //descriptorWriteCount
				writes.data(), //pDescriptorWrites
				0, //descriptorCopyCount
				nullptr //pDescriptorCopies
			);
		}
	}

	textures.resize(1);
	textures[0] = std::make_shared<Image2D>(Files::Path("../textures/default_gray.png"));

	{ // create the texture descriptor pool
		uint32_t per_texture = uint32_t(textures.size()); //for easier-to-read counting
		assert(per_texture == 1);
		std::array< VkDescriptorPoolSize, 1> pool_sizes{
			VkDescriptorPoolSize{
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1 * 1 * per_texture, //one descriptor per set, one set per texture
			},
		};

		VkDescriptorPoolCreateInfo create_info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags = 0, //because CREATE_FREE_DESCRIPTOR_SET_BIT isn't included, *can't* free individual descriptors allocated from this pool
			.maxSets = 1 * per_texture, //one set per texture
			.poolSizeCount = uint32_t(pool_sizes.size()),
			.pPoolSizes = pool_sizes.data(),
		};

		VulkanContext::VK_CHECK(
			vkCreateDescriptorPool(VulkanContext::GetDevice(), &create_info, nullptr, &texture_descriptor_pool),
			"[vulkan] Allocating descriptor pool failed");
	}

	{ //allocate and write the texture descriptor sets
		//allocate the descriptors (using the same alloc_info):
		VkDescriptorSetAllocateInfo alloc_info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = texture_descriptor_pool,
			.descriptorSetCount = 1,
			.pSetLayouts = &set2_TEXTURE,
		};
		texture_descriptors.assign(1, VK_NULL_HANDLE);
		for (VkDescriptorSet& descriptor_set : texture_descriptors) {
			VulkanContext::VK_CHECK(
				vkAllocateDescriptorSets(VulkanContext::GetDevice(), &alloc_info, &descriptor_set),
				"[vulkan] Allocating descriptor set failed");
		}

		//write descriptors for textures:
		std::vector< VkWriteDescriptorSet > writes(textures.size());

		for (auto const& image : textures) {
			size_t i = &image - &textures[0];
			writes[i] = image->getWriteDescriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER).getWriteDescriptorSet();
			writes[i].dstSet = texture_descriptors[i];
		}

		vkUpdateDescriptorSets(VulkanContext::GetDevice(), uint32_t(writes.size()), writes.data(), 0, nullptr);
	}
}

void ObjectPipeline::PushSceneDrawInfo(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId)
{
	Workspace& workspace = workspaces[surfaceId];

	//upload world info:
	{ 
		//host-side copy into World_src:
		memcpy(workspace.World_src.data(), scene->sceneUniform(), scene->sceneUniformSize());

		//add device-side copy from World_src -> World:
		assert(workspace.World_src.getSize() == workspace.World.getSize());
		Buffer::CopyBuffer(
			commandBuffer.getCommandBuffer(),
			workspace.World_src.getBuffer(),
			workspace.World.getBuffer(),
			workspace.World_src.getSize()
		);
	}

	const std::vector<ObjectInstance>& sceneObjectInstances = scene->objectInstances();

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

			std::array< VkWriteDescriptorSet, 1 > writes{
				VkWriteDescriptorSet{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = workspace.Transforms_descriptors,
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					.pBufferInfo = &Transforms_info,
				},
			};

			vkUpdateDescriptorSets(
				VulkanContext::GetDevice(),
				uint32_t(writes.size()), writes.data(), //descriptorWrites count, data
				0, nullptr //descriptorCopies count, data
			);

			std::cout << "Re-allocated object transforms buffers to " << new_bytes << " bytes." << std::endl;
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

	//draw all instances:
	const std::vector<ObjectInstance>& sceneObjectInstances = scene->objectInstances();
	Mesh* previouslyBindedMesh = nullptr;

	//std::cout << "Drawing: " << sceneObjectInstances.size() << std::endl;
	for (ObjectInstance const& inst : sceneObjectInstances)
	{
		//std::cout << glm::to_string(inst.m_TransformUniform.modelMatrix) << std::endl;
		uint32_t index = uint32_t(&inst - &sceneObjectInstances[0]);

		//bind texture descriptor set:
		vkCmdBindDescriptorSets(
			commandBuffer, //command buffer
			VK_PIPELINE_BIND_POINT_GRAPHICS, //pipeline bind point
			m_PipelineLayout, //pipeline layout
			2, //second set
			1, &texture_descriptors[0], //descriptor sets count, ptr
			0, nullptr //dynamic offsets count, ptr
		);

		bool draw = true;
		if (inst.mesh != previouslyBindedMesh) 
		{
			draw = inst.BindMesh(commandBuffer, index);
			Vertex::Bind(commandBuffer);
		}
		previouslyBindedMesh = inst.mesh;

		if (draw)
			inst.Draw(commandBuffer, index);
	}
}
