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

#include "backend/pipeline/material_pipeline/LambertianMaterialPipeline.hpp"
#include "renderer/materials/MaterialLibrary.hpp"

#include "backend/images/ImageCube.hpp"

#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, perspective 


ObjectPipeline::ObjectPipeline()
{
	m_Renderpass = std::make_unique<Renderpass>(true);
	Image2D::Create(Files::Path("../textures/default.png"));
	ImageCube::Create(Files::Path("../scene/examples/env-cube.png"), ".png");
}

ObjectPipeline::~ObjectPipeline()
{
	for (Workspace& workspace : workspaces) 
	{
		workspace.TransformsPersistent.Destroy();
		workspace.WorldPersistent.Destroy();
	}
	workspaces.clear();

	m_DescriptorAllocator.Cleanup(); // destroy pool and sets
	m_DescriptorLayoutCache.Cleanup(); // destroy all set layouts
	
	m_MaterialPipelines.clear();
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
		vkCreateRenderPass(VulkanContext::GetDevice(), &create_info, nullptr, &m_Renderpass->renderpass),
		"[Vulkan] Create Render pass failed"
	);
}

void ObjectPipeline::CreateDescriptors()
{
	workspaces.resize(VulkanContext::Get()->getWorkspaceSize());

	// create world and transform descriptor
	for (Workspace& workspace : workspaces)
	{
		workspace.WorldPersistent = Buffer(
			sizeof(Scene::SceneUniform),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			Buffer::Mapped
		);

		VkDescriptorBufferInfo World_info
		{
			.buffer = workspace.WorldPersistent.getBuffer(),
			.offset = 0,
			.range = workspace.WorldPersistent.getSize(),
		};

		DescriptorBuilder::Start(&m_DescriptorLayoutCache, &m_DescriptorAllocator)
			.BindBuffer(0, &World_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.Build(workspace.set0_World, set0_WorldLayout);

		VkDescriptorBufferInfo Transforms_info = CreateTransformStorageBuffer(workspace, 4096);
		DescriptorBuilder::Start(&m_DescriptorLayoutCache, &m_DescriptorAllocator)
			.BindBuffer(0, &Transforms_info, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.Build(workspace.set1_Transforms, set1_TransformsLayout);
	}

	// https://github.com/SaschaWillems/Vulkan/blob/master/examples/descriptorindexing/descriptorindexing.cpp#L127
	{ // create set 3: textures
		// [POI] The fragment shader will be using an unsized array of samplers, which has to be marked with the VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT
		std::vector<VkDescriptorBindingFlagsEXT> descriptorBindingFlags = {
			VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT,
		};
		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT,
			.bindingCount = 1,
			.pBindingFlags = descriptorBindingFlags.data()
		};
		
		const auto& textures = MaterialLibrary::Get()->GetTextures();

		std::vector<uint32_t> variableDesciptorCounts = {
			static_cast<uint32_t>(textures.size())
		};

		VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variableDescriptorInfoAI = 
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
			.descriptorSetCount = static_cast<uint32_t>(variableDesciptorCounts.size()),
			.pDescriptorCounts = variableDesciptorCounts.data(),
		};

		// grab all texture information
		std::vector<VkDescriptorImageInfo> textureDescriptors(textures.size());
		for (uint32_t i = 0; i < textures.size(); ++i)
		{
			auto& tex = textures[i];
			textureDescriptors[i].sampler = tex->getSampler();
			textureDescriptors[i].imageView = tex->getView();
			textureDescriptors[i].imageLayout = tex->getLayout();
		}

		// actually build the descriptor set now
		DescriptorBuilder::Start(&m_DescriptorLayoutCache, &m_DescriptorAllocator)
			.BindImage(0, textureDescriptors.data(), 
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, static_cast<uint32_t>(textures.size()))
			.Build(set2_Textures, set2_TexturesLayout, &setLayoutBindingFlags,
#if (defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT))
			// SRS - increase the per-stage descriptor samplers limit on macOS (maxPerStageDescriptorUpdateAfterBindSamplers > maxPerStageDescriptorSamplers)
			VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT
#else
			0 /*VkDescriptorSetLayoutCreateFlags*/
#endif
			, &variableDescriptorInfoAI);
	}
}

void ObjectPipeline::Rebuild()
{
	m_Renderpass->Rebuild();
}

void ObjectPipeline::CreatePipeline()
{
	m_MaterialPipelines.resize(1);
	m_MaterialPipelines[0] = std::make_unique<LambertianMaterialPipeline>(this);

	s_LinesPipeline = std::make_unique<LinesPipeline>(this);

	CreateDescriptors();

	m_MaterialPipelines[0]->Create();
	s_LinesPipeline->CreatePipeline();
}

void ObjectPipeline::Render(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId)
{
	Prepare(scene, commandBuffer, surfaceId);
	m_Renderpass->Begin(commandBuffer);
	RenderPass(scene, commandBuffer, surfaceId);
	m_Renderpass->End(commandBuffer);
}

void ObjectPipeline::Update(const Scene* scene)
{

}

void ObjectPipeline::Prepare(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId)
{
	Workspace& workspace = workspaces[surfaceId];

	//upload world info:
	memcpy(workspace.WorldPersistent.data(), scene->getSceneUniformPtr(), scene->getSceneUniformSize());

	const std::vector<ObjectInstance>& sceneObjectInstances = scene->getObjectInstances();

	//upload object transforms and allocate needed bytes
	if (!sceneObjectInstances.empty()) 
	{
		size_t needed_bytes = sceneObjectInstances.size() * sizeof(ObjectInstance::TransformUniform);
		if (workspace.TransformsPersistent.getBuffer() == VK_NULL_HANDLE 
			|| workspace.TransformsPersistent.getSize() < needed_bytes) 
		{
			//round to next multiple of 4k to avoid re-allocating continuously if vertex count grows slowly:
			size_t new_bytes = ((needed_bytes + 4096) / 4096) * 4096;

			workspace.TransformsPersistent.Destroy();

			//update the descriptor set:
			VkDescriptorBufferInfo Transforms_info = CreateTransformStorageBuffer(workspace, new_bytes);

			DescriptorBuilder::Start(&m_DescriptorLayoutCache, &m_DescriptorAllocator)
				.BindBuffer(0, &Transforms_info, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
				.Write(workspace.set1_Transforms);
		}

		assert(workspace.TransformsPersistent.getSize() >= needed_bytes);

		//copy transforms into Transforms_src:
		assert(workspace.TransformsPersistent.data() != nullptr);
		ObjectInstance::TransformUniform* out = reinterpret_cast<ObjectInstance::TransformUniform*>(workspace.TransformsPersistent.data());
		for (ObjectInstance const& inst : sceneObjectInstances) {
			*out = inst.m_TransformUniform;
			++out;
		}
	}
	
	if (UseGizmos) {
		s_LinesPipeline->Prepare(scene, commandBuffer, surfaceId);
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
}

VkDescriptorBufferInfo ObjectPipeline::CreateTransformStorageBuffer(Workspace& workspace, size_t new_bytes)
{
	workspace.TransformsPersistent = Buffer(
		new_bytes,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, //going to have GPU copy from this memory
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //host-visible memory, coherent (no special sync needed)
		Buffer::Mapped //get a pointer to the memory
	);

	//update the descriptor set:
	VkDescriptorBufferInfo Transforms_info{
		.buffer = workspace.TransformsPersistent.getBuffer(),
		.offset = 0,
		.range = workspace.TransformsPersistent.getSize(),
	};
	std::cout << "Re-allocated object transforms buffers to " << new_bytes << " bytes." << std::endl;
	return Transforms_info;
}

//draw with the objects pipeline:
void ObjectPipeline::RenderPass(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId)
{
	m_MaterialPipelines[0]->BindPipeline(commandBuffer);
	m_MaterialPipelines[0]->BindDescriptors(commandBuffer, surfaceId);

	//draw all instances in relation to a certain material:
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
		draw.material->Push(commandBuffer, m_MaterialPipelines[0]->m_PipelineLayout);

		VkDeviceSize offset = draw.first * sizeof(VkDrawIndexedIndirectCommand);
		uint32_t stride = sizeof(VkDrawIndexedIndirectCommand);

		vkCmdDrawIndexedIndirect(commandBuffer, VulkanContext::Get()->getIndirectBuffer()->getBuffer(), offset, draw.count, stride);
	}

	// draw lines
	if (UseGizmos) {
		s_LinesPipeline->Render(scene, commandBuffer, surfaceId);
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