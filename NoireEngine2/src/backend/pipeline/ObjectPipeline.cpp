#include "ObjectPipeline.hpp"

#include <iostream>
#include <array>

#include "backend/VulkanContext.hpp"
#include "math/Math.hpp"
#include "backend/shader/VulkanShader.h"

static uint32_t vert_code[] =
#include "spv/objects.vert.inl"
;

static uint32_t frag_code[] =
#include "spv/objects.frag.inl"
;

ObjectPipeline::ObjectPipeline(VkRenderPass& renderpass)
{
	CreatePipeline(renderpass, 0);
	CreateWorkspaces();
}

ObjectPipeline::~ObjectPipeline()
{
	VkDevice device = VulkanContext::GetDevice();

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

	for (Workspace& workspace : workspaces) {
		if (workspace.Transforms_src.getBuffer() != VK_NULL_HANDLE) {
			workspace.Transforms_src.Destroy();
		}
		if (workspace.Transforms.getBuffer() != VK_NULL_HANDLE) {
			workspace.Transforms.Destroy();
		}
		if (workspace.World.getBuffer() != VK_NULL_HANDLE) {
			workspace.World.Destroy();
		}
		if (workspace.World_src.getBuffer() != VK_NULL_HANDLE) {
			workspace.World_src.Destroy();
		}

	}
}

void ObjectPipeline::CreateShaders()
{
}

void ObjectPipeline::CreateDescriptors()
{
}

void ObjectPipeline::CreatePipeline(VkRenderPass& renderpass, uint32_t subpass)
{
	VulkanShader vert_module(vert_code);
	VulkanShader frag_module(frag_code);

	//shader code for vertex and fragment pipeline stages:
	std::array< VkPipelineShaderStageCreateInfo, 2 > stages{
		VkPipelineShaderStageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vert_module,
			.pName = "main"
		},
		VkPipelineShaderStageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = frag_module,
			.pName = "main"
		},
	};

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

	{
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
	
	{
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
			.pVertexInputState = &Vertex::array_input_state,
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

	{ //create descriptor pool:
		uint32_t numWorkspace = VulkanContext::Get().getWorkspaceSize(); //for easier-to-read counting

		//we only need uniform buffer descriptors for the moment:
		std::array< VkDescriptorPoolSize, 2> pool_sizes{
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
}

void ObjectPipeline::Render(const CommandBuffer& commandBuffer, uint32_t surfaceId)
{
	////get more convenient names for the current workspace and target framebuffer:
	//Workspace& workspace = workspaces[surfaceId];
	//VkFramebuffer framebuffer = swapchain_framebuffers[render_params.image_index];

	////record (into `workspace.command_buffer`) commands that run a `render_pass` that just clears `framebuffer`:
	//VK(vkResetCommandBuffer(workspace.command_buffer, 0));
	//{ //begin recording:
	//	VkCommandBufferBeginInfo begin_info{
	//		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	//		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, //will record again every submit
	//	};
	//	VK(vkBeginCommandBuffer(workspace.command_buffer, &begin_info));
	//}

	//if (!lines_vertices.empty()) { //upload lines vertices:
	//	//[re-]allocate lines buffers if needed:
	//	size_t needed_bytes = lines_vertices.size() * sizeof(lines_vertices[0]);
	//	if (workspace.lines_vertices_src.handle == VK_NULL_HANDLE || workspace.lines_vertices_src.size < needed_bytes) {
	//		//round to next multiple of 4k to avoid re-allocating continuously if vertex count grows slowly:
	//		size_t new_bytes = ((needed_bytes + 4096) / 4096) * 4096;

	//		workspace.lines_vertices_src = rtg.helpers.create_buffer(
	//			new_bytes,
	//			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, //going to have GPU copy from this memory
	//			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //host-visible memory, coherent (no special sync needed)
	//			Helpers::Mapped //get a pointer to the memory
	//		);
	//		workspace.lines_vertices = rtg.helpers.create_buffer(
	//			new_bytes,
	//			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, //going to use as vertex buffer, also going to have GPU into this memory
	//			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, //GPU-local memory
	//			Helpers::Unmapped //don't get a pointer to the memory
	//		);

	//		std::cout << "Re-allocated lines buffers to " << new_bytes << " bytes." << std::endl;
	//	}

	//	assert(workspace.lines_vertices_src.size == workspace.lines_vertices.size);
	//	assert(workspace.lines_vertices_src.size >= needed_bytes);

	//	//host-side copy into lines_vertices_src:
	//	assert(workspace.lines_vertices_src.allocation.mapped);
	//	std::memcpy(workspace.lines_vertices_src.allocation.data(), lines_vertices.data(), needed_bytes);

	//	//device-side copy from lines_vertices_src -> lines_vertices:
	//	VkBufferCopy copy_region{
	//		.srcOffset = 0,
	//		.dstOffset = 0,
	//		.size = needed_bytes,
	//	};
	//	vkCmdCopyBuffer(workspace.command_buffer, workspace.lines_vertices_src.handle, workspace.lines_vertices.handle, 1, &copy_region);
	//}

	//{ //upload camera info:
	//	LinesPipeline::Camera camera{
	//		.CLIP_FROM_WORLD = CLIP_FROM_WORLD
	//	};
	//	assert(workspace.Camera_src.size == sizeof(camera));

	//	//host-side copy into Camera_src:
	//	memcpy(workspace.Camera_src.allocation.data(), &camera, sizeof(camera));

	//	//add device-side copy from Camera_src -> Camera:
	//	assert(workspace.Camera_src.size == workspace.Camera.size);
	//	VkBufferCopy copy_region{
	//		.srcOffset = 0,
	//		.dstOffset = 0,
	//		.size = workspace.Camera_src.size,
	//	};
	//	vkCmdCopyBuffer(workspace.command_buffer, workspace.Camera_src.handle, workspace.Camera.handle, 1, &copy_region);
	//}

	//{ //upload world info:
	//	assert(workspace.Camera_src.size == sizeof(world));

	//	//host-side copy into World_src:
	//	memcpy(workspace.World_src.allocation.data(), &world, sizeof(world));

	//	//add device-side copy from World_src -> World:
	//	assert(workspace.World_src.size == workspace.World.size);
	//	VkBufferCopy copy_region{
	//		.srcOffset = 0,
	//		.dstOffset = 0,
	//		.size = workspace.World_src.size,
	//	};
	//	vkCmdCopyBuffer(workspace.command_buffer, workspace.World_src.handle, workspace.World.handle, 1, &copy_region);
	//}

	//if (!object_instances.empty()) { //upload object transforms:
	//	size_t needed_bytes = object_instances.size() * sizeof(ObjectsPipeline::Transform);
	//	if (workspace.Transforms_src.handle == VK_NULL_HANDLE || workspace.Transforms_src.size < needed_bytes) {
	//		//round to next multiple of 4k to avoid re-allocating continuously if vertex count grows slowly:
	//		size_t new_bytes = ((needed_bytes + 4096) / 4096) * 4096;
	//		if (workspace.Transforms_src.handle) {
	//			rtg.helpers.destroy_buffer(std::move(workspace.Transforms_src));
	//		}
	//		if (workspace.Transforms.handle) {
	//			rtg.helpers.destroy_buffer(std::move(workspace.Transforms));
	//		}
	//		workspace.Transforms_src = rtg.helpers.create_buffer(
	//			new_bytes,
	//			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, //going to have GPU copy from this memory
	//			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //host-visible memory, coherent (no special sync needed)
	//			Helpers::Mapped //get a pointer to the memory
	//		);
	//		workspace.Transforms = rtg.helpers.create_buffer(
	//			new_bytes,
	//			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, //going to use as storage buffer, also going to have GPU into this memory
	//			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, //GPU-local memory
	//			Helpers::Unmapped //don't get a pointer to the memory
	//		);

	//		//update the descriptor set:
	//		VkDescriptorBufferInfo Transforms_info{
	//			.buffer = workspace.Transforms.handle,
	//			.offset = 0,
	//			.range = workspace.Transforms.size,
	//		};

	//		std::array< VkWriteDescriptorSet, 1 > writes{
	//			VkWriteDescriptorSet{
	//				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	//				.dstSet = workspace.Transforms_descriptors,
	//				.dstBinding = 0,
	//				.dstArrayElement = 0,
	//				.descriptorCount = 1,
	//				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	//				.pBufferInfo = &Transforms_info,
	//			},
	//		};

	//		vkUpdateDescriptorSets(
	//			rtg.device,
	//			uint32_t(writes.size()), writes.data(), //descriptorWrites count, data
	//			0, nullptr //descriptorCopies count, data
	//		);

	//		std::cout << "Re-allocated object transforms buffers to " << new_bytes << " bytes." << std::endl;
	//	}

	//	assert(workspace.Transforms_src.size == workspace.Transforms.size);
	//	assert(workspace.Transforms_src.size >= needed_bytes);

	//	{ //copy transforms into Transforms_src:
	//		assert(workspace.Transforms_src.allocation.mapped);
	//		ObjectsPipeline::Transform* out = reinterpret_cast<ObjectsPipeline::Transform*>(workspace.Transforms_src.allocation.data());
	//		for (ObjectInstance const& inst : object_instances) {
	//			*out = inst.transform;
	//			++out;
	//		}
	//	}

	//	//device-side copy from Transforms_src -> Transforms:
	//	VkBufferCopy copy_region{
	//		.srcOffset = 0,
	//		.dstOffset = 0,
	//		.size = needed_bytes,
	//	};
	//	vkCmdCopyBuffer(workspace.command_buffer, workspace.Transforms_src.handle, workspace.Transforms.handle, 1, &copy_region);
	//}

	//{ //memory barrier to make sure copies complete before rendering happens:
	//	VkMemoryBarrier memory_barrier{
	//		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
	//		.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
	//		.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
	//	};

	//	vkCmdPipelineBarrier(workspace.command_buffer,
	//		VK_PIPELINE_STAGE_TRANSFER_BIT, //srcStageMask
	//		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, //dstStageMask
	//		0, //dependencyFlags
	//		1, &memory_barrier, //memoryBarriers (count, data)
	//		0, nullptr, //bufferMemoryBarriers (count, data)
	//		0, nullptr //imageMemoryBarriers (count, data)
	//	);
	//}

	//{ //render pass
	//	std::array< VkClearValue, 2 > clear_values{
	//		VkClearValue{.color{.float32{1.0f, 0.5f, 0.5f, 1.0f} } },
	//		VkClearValue{.depthStencil{.depth = 1.0f, .stencil = 0 } },
	//	};

	//	VkRenderPassBeginInfo begin_info{
	//		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
	//		.renderPass = render_pass,
	//		.framebuffer = framebuffer,
	//		.renderArea{
	//			.offset = {.x = 0, .y = 0},
	//			.extent = rtg.swapchain_extent,
	//		},
	//		.clearValueCount = uint32_t(clear_values.size()),
	//		.pClearValues = clear_values.data(),
	//	};

	//	vkCmdBeginRenderPass(workspace.command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

	//	{ //set scissor rectangle:
	//		VkRect2D scissor{
	//			.offset = {.x = 0, .y = 0},
	//			.extent = rtg.swapchain_extent,
	//		};
	//		vkCmdSetScissor(workspace.command_buffer, 0, 1, &scissor);
	//	}
	//	{ //configure viewport transform:
	//		VkViewport viewport{
	//			.x = 0.0f,
	//			.y = 0.0f,
	//			.width = float(rtg.swapchain_extent.width),
	//			.height = float(rtg.swapchain_extent.height),
	//			.minDepth = 0.0f,
	//			.maxDepth = 1.0f,
	//		};
	//		vkCmdSetViewport(workspace.command_buffer, 0, 1, &viewport);
	//	}

	//	{ //draw with the background pipeline:
	//		vkCmdBindPipeline(workspace.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, background_pipeline.handle);

	//		{ //push time:
	//			BackgroundPipeline::Push push{
	//				.time = float(time),
	//			};
	//			vkCmdPushConstants(workspace.command_buffer, background_pipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);
	//		}

	//		vkCmdDraw(workspace.command_buffer, 3, 1, 0, 0);
	//	}

	//	{ //draw with the lines pipeline:
	//		vkCmdBindPipeline(workspace.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lines_pipeline.handle);

	//		{ //use lines_vertices (offset 0) as vertex buffer binding 0:
	//			std::array< VkBuffer, 1 > vertex_buffers{ workspace.lines_vertices.handle };
	//			std::array< VkDeviceSize, 1 > offsets{ 0 };
	//			vkCmdBindVertexBuffers(workspace.command_buffer, 0, uint32_t(vertex_buffers.size()), vertex_buffers.data(), offsets.data());
	//		}

	//		{ //bind Camera descriptor set:
	//			std::array< VkDescriptorSet, 1 > descriptor_sets{
	//				workspace.Camera_descriptors, //0: Camera
	//			};
	//			vkCmdBindDescriptorSets(
	//				workspace.command_buffer, //command buffer
	//				VK_PIPELINE_BIND_POINT_GRAPHICS, //pipeline bind point
	//				lines_pipeline.layout, //pipeline layout
	//				0, //first set
	//				uint32_t(descriptor_sets.size()), descriptor_sets.data(), //descriptor sets count, ptr
	//				0, nullptr //dynamic offsets count, ptr
	//			);
	//		}

	//		//draw lines vertices:
	//		vkCmdDraw(workspace.command_buffer, uint32_t(lines_vertices.size()), 1, 0, 0);
	//	}

	//	{ //draw with the objects pipeline:
	//		vkCmdBindPipeline(workspace.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, objects_pipeline.handle);

	//		{ //use object_vertices (offset 0) as vertex buffer binding 0:
	//			std::array< VkBuffer, 1 > vertex_buffers{ object_vertices.handle };
	//			std::array< VkDeviceSize, 1 > offsets{ 0 };
	//			vkCmdBindVertexBuffers(workspace.command_buffer, 0, uint32_t(vertex_buffers.size()), vertex_buffers.data(), offsets.data());
	//		}

	//		{ //bind Transforms descriptor set:
	//			std::array< VkDescriptorSet, 2 > descriptor_sets{
	//				workspace.World_descriptors, //0: World
	//				workspace.Transforms_descriptors, //1: Transforms
	//			};
	//			vkCmdBindDescriptorSets(
	//				workspace.command_buffer, //command buffer
	//				VK_PIPELINE_BIND_POINT_GRAPHICS, //pipeline bind point
	//				objects_pipeline.layout, //pipeline layout
	//				0, //first set
	//				uint32_t(descriptor_sets.size()), descriptor_sets.data(), //descriptor sets count, ptr
	//				0, nullptr //dynamic offsets count, ptr
	//			);
	//		}

	//		//Camera descriptor set is still bound, but unused(!)

	//		//draw all instances:
	//		for (ObjectInstance const& inst : object_instances) {
	//			uint32_t index = uint32_t(&inst - &object_instances[0]);

	//			//bind texture descriptor set:
	//			vkCmdBindDescriptorSets(
	//				workspace.command_buffer, //command buffer
	//				VK_PIPELINE_BIND_POINT_GRAPHICS, //pipeline bind point
	//				objects_pipeline.layout, //pipeline layout
	//				2, //second set
	//				1, &texture_descriptors[inst.texture], //descriptor sets count, ptr
	//				0, nullptr //dynamic offsets count, ptr
	//			);

	//			vkCmdDraw(workspace.command_buffer, inst.vertices.count, 1, inst.vertices.first, index);
	//		}

	//		//draw all vertices:
	//		vkCmdDraw(workspace.command_buffer, uint32_t(object_vertices.size / sizeof(ObjectsPipeline::Vertex)), 1, 0, 0);
	//	}

	//	vkCmdEndRenderPass(workspace.command_buffer);
	//}

	//VK(vkEndCommandBuffer(workspace.command_buffer));

	////submit `workspace.command buffer` for the GPU to run:
	//{
	//	std::array< VkSemaphore, 1 > wait_semaphores{
	//	render_params.image_available
	//	};
	//	std::array< VkPipelineStageFlags, 1 > wait_stages{
	//		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	//	};
	//	static_assert(wait_semaphores.size() == wait_stages.size(), "every semaphore needs a stage");

	//	std::array< VkSemaphore, 1 > signal_semaphores{
	//		render_params.image_done
	//	};
	//	VkSubmitInfo submit_info{
	//		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	//		.waitSemaphoreCount = uint32_t(wait_semaphores.size()),
	//		.pWaitSemaphores = wait_semaphores.data(),
	//		.pWaitDstStageMask = wait_stages.data(),
	//		.commandBufferCount = 1,
	//		.pCommandBuffers = &workspace.command_buffer,
	//		.signalSemaphoreCount = uint32_t(signal_semaphores.size()),
	//		.pSignalSemaphores = signal_semaphores.data(),
	//	};

	//	VK(vkQueueSubmit(rtg.graphics_queue, 1, &submit_info, render_params.workspace_available));
	//}
}

void ObjectPipeline::CreateWorkspaces()
{
	workspaces.resize(VulkanContext::Get().getWorkspaceSize());
	
	for (Workspace& workspace : workspaces) 
	{
		workspace.World_src = Buffer(
			sizeof(World),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			Buffer::Mapped
		);
		workspace.World = Buffer(
			sizeof(World),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			Buffer::Mapped
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
			//NOTE: will actually fill in this descriptor set just a bit lower
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

	{ //create object vertices
		std::vector<Vertex> vertices;

		constexpr float R2 = 1.0f; //tube radius

		constexpr uint32_t U_STEPS = 20;
		constexpr uint32_t V_STEPS = 16;

		//texture repeats around the torus:
		constexpr float V_REPEATS = 2.0f;
		float U_REPEATS = std::ceil(V_REPEATS / R2);

		auto emplace_vertex = [&](uint32_t ui, uint32_t vi) {
			//convert steps to angles:
			// (doing the mod since trig on 2 M_PI may not exactly match 0)
			float ua = (ui % U_STEPS) / float(U_STEPS) * 2.0f * Math::PI<float>;
			float va = (vi % V_STEPS) / float(V_STEPS) * 2.0f * Math::PI<float>;

			float x = (R2 * std::cos(va)) * std::cos(ua);
			float y = (R2 * std::cos(va)) * std::sin(ua);
			float z = R2 * std::sin(va);
			float L = std::sqrt(x * x + y * y + z * z);

			vertices.emplace_back(PosNorTexVertex{
				.Position{x, y, z},
				.Normal{
					.x = x / L,
					.y = y / L,
					.z = z / L,
				},
				.TexCoord{
					.s = ui / float(U_STEPS) * U_REPEATS,
					.t = vi / float(V_STEPS) * V_REPEATS,
				},
				});
			};

		for (uint32_t ui = 0; ui < U_STEPS; ++ui) {
			for (uint32_t vi = 0; vi < V_STEPS; ++vi) {
				emplace_vertex(ui, vi);
				emplace_vertex(ui + 1, vi);
				emplace_vertex(ui, vi + 1);

				emplace_vertex(ui, vi + 1);
				emplace_vertex(ui + 1, vi);
				emplace_vertex(ui + 1, vi + 1);
			}
		}

		size_t bytes = vertices.size() * sizeof(vertices[0]);

		vertexBuffer = Buffer(
			bytes,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		//copy data to buffer:
		Buffer::TransferToBuffer(vertices.data(), bytes, vertexBuffer.getBuffer());
	}
}
