#include "ObjectPipeline.hpp"

#include <array>

#include "backend/VulkanContext.hpp"
#include "backend/shader/VulkanShader.h"
#include "math/Math.hpp"
#include "core/Time.hpp"

#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, perspective 

static uint32_t vert_code[] =
#include "spv/shaders/objects.vert.inl"
;

static uint32_t frag_code[] =
#include "spv/shaders/objects.frag.inl"
;

ObjectPipeline::ObjectPipeline(Renderer* renderer) :
	VulkanPipeline(renderer)
{
	CreatePipeline(m_BindedRenderer->m_Renderpass, 0);
	CreateWorkspaces();
}

ObjectPipeline::~ObjectPipeline()
{
	VkDevice device = VulkanContext::GetDevice();

	if (texture_descriptor_pool) {
		vkDestroyDescriptorPool(device, texture_descriptor_pool, nullptr);
		texture_descriptor_pool = nullptr;

		//this also frees the descriptor sets allocated from the pool:
		texture_descriptors.clear();
	}

	textures.clear();

	if (vertexBuffer.getBuffer() != VK_NULL_HANDLE)
		vertexBuffer.Destroy();

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
		assert(numWorkspace == 1);
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
}

void ObjectPipeline::Render(const CommandBuffer& commandBuffer, uint32_t surfaceId)
{
	//get more convenient names for the current workspace and target framebuffer:
	Workspace& workspace = workspaces[surfaceId];
	VkFramebuffer framebuffer = m_BindedRenderer->m_Framebuffers[VulkanContext::Get().getCurrentFrame()];

	{ //upload world info:
		//host-side copy into World_src:
		memcpy(workspace.World_src.data(), &world, sizeof(world));

		//add device-side copy from World_src -> World:
		assert(workspace.World_src.getSize() == workspace.World.getSize());
		VkBufferCopy copy_region{
			.srcOffset = 0,
			.dstOffset = 0,
			.size = workspace.World_src.getSize(),
		};
		vkCmdCopyBuffer(commandBuffer, workspace.World_src.getBuffer(), workspace.World.getBuffer(), 1, &copy_region);
	}

	if (!object_instances.empty()) { //upload object transforms:
		size_t needed_bytes = object_instances.size() * sizeof(Transform);
		if (workspace.Transforms_src.getBuffer() == VK_NULL_HANDLE || workspace.Transforms_src.getSize() < needed_bytes) {
			//round to next multiple of 4k to avoid re-allocating continuously if vertex count grows slowly:
			size_t new_bytes = ((needed_bytes + 4096) / 4096) * 4096;
			if (workspace.Transforms_src.getBuffer()) {
				workspace.Transforms_src.Destroy();
			}
			if (workspace.Transforms.getBuffer()) {
				workspace.Transforms.Destroy();
			}
			workspace.Transforms_src = Buffer(
				new_bytes,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, //going to have GPU copy from this memory
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //host-visible memory, coherent (no special sync needed)
				Buffer::Mapped //get a pointer to the memory
			);
			workspace.Transforms = Buffer(
				new_bytes,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, //going to use as storage buffer, also going to have GPU into this memory
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, //GPU-local memory
				Buffer::Unmapped //don't get a pointer to the memory
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
			Transform* out = reinterpret_cast<Transform*>(workspace.Transforms_src.data());
			for (ObjectInstance const& inst : object_instances) {
				*out = inst.transform;
				++out;
			}
		}

		Buffer::CopyBuffer(commandBuffer, workspace.Transforms_src.getBuffer(), workspace.Transforms.getBuffer(), needed_bytes);
	}

	{ //memory barrier to make sure copies complete before rendering happens:
		VkMemoryBarrier memory_barrier{
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
		};

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, //srcStageMask
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, //dstStageMask
			0, //dependencyFlags
			1, &memory_barrier, //memoryBarriers (count, data)
			0, nullptr, //bufferMemoryBarriers (count, data)
			0, nullptr //imageMemoryBarriers (count, data)
		);
	}

	{ //render pass
		std::array< VkClearValue, 2 > clear_values{
			VkClearValue{.color{.float32{1.0f, 0.5f, 0.5f, 1.0f} } },
			VkClearValue{.depthStencil{.depth = 1.0f, .stencil = 0 } },
		};

		VkRenderPassBeginInfo begin_info{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = m_BindedRenderer->m_Renderpass,
			.framebuffer = framebuffer,
			.renderArea{
				.offset = {.x = 0, .y = 0},
				.extent = VulkanContext::Get().getSwapChain()->getExtent(),
			},
			.clearValueCount = uint32_t(clear_values.size()),
			.pClearValues = clear_values.data(),
		};

		vkCmdBeginRenderPass(commandBuffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

		{ //set scissor rectangle:
			VkRect2D scissor{
				.offset = {.x = 0, .y = 0},
				.extent = VulkanContext::Get().getSwapChain()->getExtent(),
			};
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		}
		{ //configure viewport transform:
			VkViewport viewport{
				.x = 0.0f,
				.y = 0.0f,
				.width = float(VulkanContext::Get().getSwapChain()->getExtent().width),
				.height = float(VulkanContext::Get().getSwapChain()->getExtent().height),
				.minDepth = 0.0f,
				.maxDepth = 1.0f,
			};
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		}

		{ //draw with the objects pipeline:
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

			{ //use object_vertices (offset 0) as vertex buffer binding 0:
				std::array< VkBuffer, 1 > vertex_buffers{ vertexBuffer.getBuffer()};
				std::array< VkDeviceSize, 1 > offsets{ 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, uint32_t(vertex_buffers.size()), vertex_buffers.data(), offsets.data());
			}

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
			for (ObjectInstance const& inst : object_instances) {
				uint32_t index = uint32_t(&inst - &object_instances[0]);

				//bind texture descriptor set:
				vkCmdBindDescriptorSets(
					commandBuffer, //command buffer
					VK_PIPELINE_BIND_POINT_GRAPHICS, //pipeline bind point
					m_PipelineLayout, //pipeline layout
					2, //second set
					1, &texture_descriptors[inst.texture], //descriptor sets count, ptr
					0, nullptr //dynamic offsets count, ptr
				);

				vkCmdDraw(commandBuffer, inst.vertices.count, 1, inst.vertices.first, index);
			}
		}

		vkCmdEndRenderPass(commandBuffer);
	}
}

inline glm::mat4 look_at(
	float eye_x, float eye_y, float eye_z,
	float target_x, float target_y, float target_z,
	float up_x, float up_y, float up_z) {

	//NOTE: this would be a lot cleaner with a vec3 type and some overloads!

	//compute vector from eye to target:
	float in_x = target_x - eye_x;
	float in_y = target_y - eye_y;
	float in_z = target_z - eye_z;

	//normalize 'in' vector:
	float inv_in_len = 1.0f / std::sqrt(in_x * in_x + in_y * in_y + in_z * in_z);
	in_x *= inv_in_len;
	in_y *= inv_in_len;
	in_z *= inv_in_len;

	//make 'up' orthogonal to 'in':
	float in_dot_up = in_x * up_x + in_y * up_y + in_z * up_z;
	up_x -= in_dot_up * in_x;
	up_y -= in_dot_up * in_y;
	up_z -= in_dot_up * in_z;

	//normalize 'up' vector:
	float inv_up_len = 1.0f / std::sqrt(up_x * up_x + up_y * up_y + up_z * up_z);
	up_x *= inv_up_len;
	up_y *= inv_up_len;
	up_z *= inv_up_len;

	//compute 'right' vector as 'in' x 'up'
	float right_x = in_y * up_z - in_z * up_y;
	float right_y = in_z * up_x - in_x * up_z;
	float right_z = in_x * up_y - in_y * up_x;

	//compute dot products of right, in, up with eye:
	float right_dot_eye = right_x * eye_x + right_y * eye_y + right_z * eye_z;
	float up_dot_eye = up_x * eye_x + up_y * eye_y + up_z * eye_z;
	float in_dot_eye = in_x * eye_x + in_y * eye_y + in_z * eye_z;

	//final matrix: (computes (right . (v - eye), up . (v - eye), -in . (v-eye), v.w )
	return glm::mat4{ //note: column-major storage order
		right_x, up_x, -in_x, 0.0f,
		right_y, up_y, -in_y, 0.0f,
		right_z, up_z, -in_z, 0.0f,
		-right_dot_eye, -up_dot_eye, in_dot_eye, 1.0f,
	};
}

void ObjectPipeline::Update()
{
	static float time = 0;
	time += Time::DeltaTime;

	{ //static sun and sky:
		world.SKY_DIRECTION.x = 0.0f;
		world.SKY_DIRECTION.y = 0.0f;
		world.SKY_DIRECTION.z = 1.0f;

		world.SKY_ENERGY.r = 0.1f;
		world.SKY_ENERGY.g = 0.1f;
		world.SKY_ENERGY.b = 0.2f;

		world.SUN_DIRECTION.x = 6.0f / 23.0f;
		world.SUN_DIRECTION.y = 13.0f / 23.0f;
		world.SUN_DIRECTION.z = -18.0f / 23.0f;

		world.SUN_ENERGY.r = 1.0f;
		world.SUN_ENERGY.g = 0.0f;
		world.SUN_ENERGY.b = 0.0f;
	}

	{ //camera orbiting the origin:
		float ang = Math::PI<float> * 2.0f * 10.0f * (time / 100.0f);
		CLIP_FROM_WORLD = glm::perspective(
			60.0f / Math::PI<float> * 180.0f, //vfov
			1920.0f / 1080, //aspect
			0.1f, //near
			1000.0f //far
		) * look_at(
			3.0f * std::cos(ang), 3.0f * std::sin(ang), 1.0f, //eye
			0.0f, 0.0f, 0.5f, //target
			0.0f, 0.0f, 1.0f //up
		);
	}

	{ //make some objects:
		object_instances.clear();

		for (int i = 0; i < 1; i++)
		{
			for (int j = 0; j < 1; j++)
			{
				for (int k = 0; k < 5; k++) {
					glm::mat4 WORLD_FROM_LOCAL{
						0.2f, 0.0f, 0.0f, 0.0f,
						0.0f, 0.2f, 0.0f, 0.0f,
						0.0f, 0.0f, 0.2f, 0.0f,
						i * 0.3f / k, j * 0.2f / k, 1.0f * k * std::sin(0.1f * time), 1.0f,
					};

					object_instances.emplace_back(ObjectInstance{
						.vertices = plane_vertices,
						.transform{
							.CLIP_FROM_LOCAL = CLIP_FROM_WORLD * WORLD_FROM_LOCAL,
							.WORLD_FROM_LOCAL = WORLD_FROM_LOCAL,
							.WORLD_FROM_LOCAL_NORMAL = WORLD_FROM_LOCAL,
						},
						.texture = 0,
						});
				}
			}
		}
	}
}

void ObjectPipeline::CreateWorkspaces()
{
	workspaces.resize(VulkanContext::Get().getWorkspaceSize());
	std::cout << "Created workspace of size " << workspaces.size() << '\n';
	assert(workspaces.size() > 0);
	
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

		plane_vertices.count = uint32_t(vertices.size()) - plane_vertices.first;
	}

	textures.resize(1);
	textures[0] = std::make_shared<Image2D>("F:/gameDev/Engines/NoireEngine2/NoireEngine2/src/textures/default.png");

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
