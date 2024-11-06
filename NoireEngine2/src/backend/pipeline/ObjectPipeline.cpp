#include "ObjectPipeline.hpp"

#include <array>
#include <algorithm>
#include <execution>
#include <thread>

#include "backend/VulkanContext.hpp"
#include "backend/shader/VulkanShader.h"

#include "math/Math.hpp"
#include "core/Time.hpp"
#include "core/Timer.hpp"
#include "utils/Logger.hpp"
#include "core/resources/Files.hpp"

#include "renderer/materials/Material.hpp"
#include "renderer/object/Mesh.hpp"
#include "renderer/scene/Scene.hpp"
#include "renderer/scene/SceneManager.hpp"

#include "backend/pipeline/material_pipeline/MaterialPipelines.hpp"
#include "renderer/materials/MaterialLibrary.hpp"

#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, perspective 

#define DEFAULT_SKYBOX "../textures/material_textures/Skybox.png"

ObjectPipeline::ObjectPipeline()
{
	m_Renderpass = std::make_unique<Renderpass>(true);
	Image2D::Create(Files::Path("../textures/default.png"), VK_FORMAT_R8G8B8A8_SRGB, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, false, false, true);
}

ObjectPipeline::~ObjectPipeline()
{
	for (Workspace& workspace : workspaces) 
	{
		workspace.Transforms_src.Destroy();
		workspace.Transforms.Destroy();
		workspace.World.Destroy();
		workspace.World_src.Destroy();
		
		for (int i = 0; i < workspace.Lights.size(); i++)
		{
			workspace.Lights_src[i].Destroy();
			workspace.Lights[i].Destroy();
		}
	}
	workspaces.clear();

	m_DescriptorAllocator.Cleanup(); // destroy pool and sets
	
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
	workspaces.resize(VulkanContext::Get()->getFramesInFlight());

	// create set 0 and set 1: world and transform descriptor
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

		// build world buffer descriptor
		DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
			.BindBuffer(0, &World_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR)
			.Build(workspace.set0_World, set0_WorldLayout);

		// build transforms and lights storage buffers, no buffer allocation
		DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // dir lights
			.AddBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // point lights
			.AddBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // spot lights
			.Build(workspace.set1_StorageBuffers, set1_StorageBuffersLayout);
	}

	// create set 2: textures with descriptor indexing
	{
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
		DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
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

	// create set 3: environment descriptors
	{
		Scene* scene = SceneManager::Get()->getScene();
		if (!scene->m_Skybox)
			scene->AddSkybox(DEFAULT_SKYBOX, Scene::SkyboxType::RGB, true);

		VkDescriptorImageInfo cubeMapInfo = scene->m_Skybox->GetDescriptorInfo();
		VkDescriptorImageInfo lambertianLUTInfo = scene->m_SkyboxLambertian->GetDescriptorInfo();
		VkDescriptorImageInfo specularBRDFInfo = scene->m_SpecularBRDF->GetDescriptorInfo();
		VkDescriptorImageInfo prefilteredEnvMapInfo = scene->m_PrefilteredEnvMap->GetDescriptorInfo();

		// first build lambertian LUT and specular BRDF info
		DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
			.BindImage(0, &cubeMapInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.BindImage(1, &lambertianLUTInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.BindImage(2, &specularBRDFInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.BindImage(3, &prefilteredEnvMapInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.Build(set3_Cubemap, set3_CubemapLayout);
	}

	// create set 4: shadow mapping with descriptor indexing
	{
		const auto& shadowPasses = s_ShadowPipeline->getShadowPasses();
		const auto& cascadePasses = s_ShadowPipeline->getCascadePasses();
		const auto& omniPasses = s_ShadowPipeline->getOmniPasses();

		NE_INFO("Found {} shadow maps.", shadowPasses.size());
		NE_INFO("Found {} cascaded shadow maps.", cascadePasses.size());
		NE_INFO("Found {} omnidirectional shadow maps.", omniPasses.size());

		std::vector<VkDescriptorBindingFlagsEXT> descriptorBindingFlags = {
			VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT,
		};
		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT,
			.bindingCount = 1,
			.pBindingFlags = descriptorBindingFlags.data()
		};

		// total number of descriptors across all shadow passes
		uint32_t combinedShadowDescriptorsCount = static_cast<uint32_t>(
			shadowPasses.size() + 
			cascadePasses.size() * SHADOW_MAP_CASCADE_COUNT +
			omniPasses.size() * OMNI_SHADOWMAPS_COUNT);

		// combine all shadow passes
		std::vector<uint32_t> variableDesciptorCounts = {
			combinedShadowDescriptorsCount
		};

		VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variableDescriptorInfoAI =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
			.descriptorSetCount = static_cast<uint32_t>(variableDesciptorCounts.size()),
			.pDescriptorCounts = variableDesciptorCounts.data(),
		};

		// grab all texture information
		std::vector<VkDescriptorImageInfo> shadowDescriptors;

		// push cascades first, since directional light is always first
		for (uint32_t i = 0; i < cascadePasses.size(); ++i)
		{
			for (int cascadeIndex = 0; cascadeIndex < SHADOW_MAP_CASCADE_COUNT; ++cascadeIndex) {
				auto& depth = cascadePasses[i].cascades[cascadeIndex].depthAttachment;
				shadowDescriptors.emplace_back(depth->getSampler(), depth->getView(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
			}
		}

		// omni point light shadowpasses here
		for (uint32_t i = 0; i < omniPasses.size(); ++i)
		{
			for (int faceIndex = 0; faceIndex < OMNI_SHADOWMAPS_COUNT; ++faceIndex) {
				auto& depth = omniPasses[i].cubefaces[faceIndex].depthAttachment;
				shadowDescriptors.emplace_back(depth->getSampler(), depth->getView(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
			}
		}

		// push spotlight shadowpasses here
		for (uint32_t i = 0; i < shadowPasses.size(); ++i)
		{
			auto& depth = shadowPasses[i].depthAttachment;
			shadowDescriptors.emplace_back(depth->getSampler(), depth->getView(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
		}

		// actually build the descriptor set now
		DescriptorBuilder builder = DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator);
		
		if (combinedShadowDescriptorsCount > 0)
			builder.BindImage(0, shadowDescriptors.data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT, combinedShadowDescriptorsCount);
		else
			builder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

		builder.Build(set4_ShadowMap, set4_ShadowMapLayout, &setLayoutBindingFlags,
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
	m_Renderpass->RebuildFromSwapchain();
}

void ObjectPipeline::CreatePipeline()
{
	m_MaterialPipelines.resize(NUM_WORKFLOWS);
	for (int i = 0; i < NUM_WORKFLOWS; i++) {
		m_MaterialPipelines[i] = MaterialPipeline::Create(Material::Workflow(i), this);
	}

	s_RaytracingPipeline = std::make_unique<RaytracingPipeline>(this);

	s_LinesPipeline = std::make_unique<LinesPipeline>(this);
	s_SkyboxPipeline = std::make_unique<SkyboxPipeline>(this);
	
	// create shadow pipeline and initialize its shadow frame buffer and render pass
	s_ShadowPipeline = std::make_unique<ShadowPipeline>(this);
	s_ShadowPipeline->CreateRenderPass();

	// this relies on shadow pipeline's depth attachment
	CreateDescriptors();

	// the following pipelines rely on ObjectPipeline's descriptor sets
	s_ShadowPipeline->CreatePipeline();

	s_RaytracingPipeline->CreatePipeline();

	for (int i = 0; i < NUM_WORKFLOWS; i++) {
		m_MaterialPipelines[i]->Create();
	}

	s_LinesPipeline->CreatePipeline();
	s_SkyboxPipeline->CreatePipeline();
}

void ObjectPipeline::Render(const Scene* scene, const CommandBuffer& commandBuffer)
{
	Prepare(scene, commandBuffer);
	PrepareIndirectDrawBuffer(scene);

	// render shadow passes
	s_ShadowPipeline->Render(scene, commandBuffer);

	// draw rtx
	s_RaytracingPipeline->Render(scene, commandBuffer);

	m_Renderpass->Begin(commandBuffer);
	{
		DrawScene(scene, commandBuffer);

		// draw lines
		if (UseGizmos)
			s_LinesPipeline->Render(scene, commandBuffer);

		s_SkyboxPipeline->Render(scene, commandBuffer);
	}
	m_Renderpass->End(commandBuffer);
}

void ObjectPipeline::Update(const Scene* scene)
{

}

void ObjectPipeline::Prepare(const Scene* scene, const CommandBuffer& commandBuffer)
{
	s_ShadowPipeline->Prepare(scene, commandBuffer);

	Workspace& workspace = workspaces[CURR_FRAME];
	
	//upload world info:
	{ 
		//host-side copy into World_src:
		memcpy(workspace.World_src.data(), scene->getSceneUniformPtr(), scene->getSceneUniformSize());

		//add device-side copy from World_src -> World:
		assert(workspace.World_src.getSize() == workspace.World.getSize());
		Buffer::CopyBuffer(commandBuffer, workspace.World_src.getBuffer(), workspace.World.getBuffer(), workspace.World_src.getSize());
	}

	//upload object transforms and allocate needed bytes
	{
		const auto& sceneObjectInstances = scene->getObjectInstances();

		size_t needed_bytes = 0;
		for (int i = 0; i < sceneObjectInstances.size(); ++i)
			needed_bytes += sceneObjectInstances[i].size() * sizeof(ObjectInstance::TransformUniform);

		// resize as neccesary
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

			DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
				.BindBuffer(0, &Transforms_info, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
				.Write(workspace.set1_StorageBuffers);

			NE_INFO("Reallocated transform to {} bytes", new_bytes);
		}

		assert(workspace.Transforms_src.getSize() == workspace.Transforms.getSize());
		assert(workspace.Transforms_src.getSize() >= needed_bytes);

		{ //copy transforms into Transforms_src:
			assert(workspace.Transforms_src.data() != nullptr);
			ObjectInstance::TransformUniform* out = reinterpret_cast<ObjectInstance::TransformUniform*>(workspace.Transforms_src.data());
			for (int i = 0; i < sceneObjectInstances.size(); ++i) {
				for (auto& inst : sceneObjectInstances[i]) {
					*out = inst.m_TransformUniform;
					++out;
				}
			}
		}

		Buffer::CopyBuffer(commandBuffer, workspace.Transforms_src.getBuffer(), workspace.Transforms.getBuffer(), workspace.Transforms_src.getSize());
	}

	// upload and allocated needed bytes for lights
	{
		const auto& lightInstances = scene->getLightInstances();

		std::array<size_t, 3> neededBytesPerLightType = { 0 };
		for (int i = 0; i < lightInstances.size(); ++i)
		{
			switch (lightInstances[i]->type)
			{
			case 0/*Light::Type::Directional*/:
				neededBytesPerLightType[0] += sizeof(DirectionalLightUniform);
				break;
			case 1/*Light::Type::Point*/:
				neededBytesPerLightType[1] += sizeof(PointLightUniform);
				break;
			case 2/*Light::Type::Spot*/:
				neededBytesPerLightType[2] += sizeof(SpotLightUniform);
				break;
			}
		}

		// iterate over all types of lights and potentially reallocate
		for (int i = 0; i < 3; i++)
		{
			Buffer& bufSrc = workspace.Lights_src[i];
			Buffer& buf = workspace.Lights[i];
			size_t neededBytes = neededBytesPerLightType[i];

			// resize as neccesary
			if (bufSrc.getBuffer() == VK_NULL_HANDLE || bufSrc.getSize() < neededBytes)
			{
				size_t new_bytes = ((neededBytes + 4096) / 4096) * 4096;

				bufSrc.Destroy();
				buf.Destroy();

				bufSrc = Buffer(
					new_bytes,
					VK_BUFFER_USAGE_TRANSFER_SRC_BIT, //going to have GPU copy from this memory
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //host-visible memory, coherent (no special sync needed)
					Buffer::Mapped //get a pointer to the memory
				);
				buf = Buffer(
					new_bytes,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, //going to use as storage buffer, also going to have GPU into this memory
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT //GPU-local memory
				);

				//update the descriptor set:
				VkDescriptorBufferInfo LightInfo{
					.buffer = buf.getBuffer(),
					.offset = 0,
					.range = buf.getSize(),
				};

				DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
					.BindBuffer(i + 1 /*cuz index 0 is transforms*/, &LightInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.Write(workspace.set1_StorageBuffers);

				NE_INFO("Reallocated light type {} to {} bytes", i, new_bytes);
			}

			assert(bufSrc.getSize() == buf.getSize());
			assert(bufSrc.getSize() >= neededBytes);
		}

		// copy into buffers
		DirectionalLightUniform* directionalIt = reinterpret_cast<DirectionalLightUniform*>(workspace.Lights_src[0].data());
		PointLightUniform* pointIt = reinterpret_cast<PointLightUniform*>(workspace.Lights_src[1].data());
		SpotLightUniform* spotIt = reinterpret_cast<SpotLightUniform*>(workspace.Lights_src[2].data());

		for (int i = 0; i < lightInstances.size(); ++i)
		{
			switch (lightInstances[i]->type)
			{
			case 0/*Light::Type::Directional*/:
				memcpy(directionalIt, lightInstances[i]->GetLightUniformAs<DirectionalLightUniform>(), sizeof(DirectionalLightUniform));
				directionalIt++;
				break;
			case 1/*Light::Type::Point*/:
				memcpy(pointIt, lightInstances[i]->GetLightUniformAs<PointLightUniform>(), sizeof(PointLightUniform));
				pointIt++;
				break;
			case 2/*Light::Type::Spot*/:
				memcpy(spotIt, lightInstances[i]->GetLightUniformAs<SpotLightUniform>(), sizeof(SpotLightUniform));
				spotIt++;
				break;
			}
		}
	
		// copy all 3
		for (int i = 0; i < 3; i++)
			Buffer::CopyBuffer(commandBuffer, workspace.Lights_src[i].getBuffer(), workspace.Lights[i].getBuffer(), workspace.Lights_src[i].getSize());
	}
	
	// prepare other pipelines

	if (UseGizmos)
		s_LinesPipeline->Prepare(scene, commandBuffer);

	s_SkyboxPipeline->Prepare(scene, commandBuffer);

	{ //memory barrier to make sure copies complete before rendering happens:
		VkMemoryBarrier memory_barrier{
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
		};

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, //srcStageMask
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, //dstStageMask
			0, //dependencyFlags
			1, &memory_barrier, //memoryBarriers (count, data)
			0, nullptr, //bufferMemoryBarriers (count, data)
			0, nullptr //imageMemoryBarriers (count, data)
		);
	}
}

void ObjectPipeline::CompactDraws(const std::vector<ObjectInstance>& objects, uint32_t workflowIndex)
{
	std::vector<IndirectBatch>& draws = m_IndirectBatches[workflowIndex];
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
}

void ObjectPipeline::PrepareIndirectDrawBuffer(const Scene* scene)
{
	const auto& allInstances = scene->getObjectInstances();
	m_IndirectBatches.resize(allInstances.size());

	ObjectsDrawn = 0;
	NumDrawCalls = 0;
	VerticesDrawn = 0;

	VkDrawIndexedIndirectCommand* drawCommands = (VkDrawIndexedIndirectCommand*)VulkanContext::Get()->getIndirectBuffer()->data();

	//draw all instances in relation to a certain material:
	uint32_t instanceIndex = 0;

	for (int workflowIndex = 0; workflowIndex < allInstances.size(); ++workflowIndex)
	{
		const auto& workflowInstances = allInstances[workflowIndex];
		if (workflowInstances.empty())
			continue;

		// make draws into compact batches
		m_IndirectBatches[workflowIndex].clear();
		CompactDraws(workflowInstances, workflowIndex);

		// encode the draw data of each object into the indirect draw buffer
		uint32_t instanceCount = (uint32_t)workflowInstances.size();
		for (uint32_t i = 0; i < instanceCount; i++)
		{
			drawCommands[instanceIndex].indexCount = workflowInstances[i].mesh->getIndexCount();
			drawCommands[instanceIndex].instanceCount = 1;
			drawCommands[instanceIndex].firstIndex = 0;
			drawCommands[instanceIndex].vertexOffset = 0;
			drawCommands[instanceIndex].firstInstance = instanceIndex;
			instanceIndex++;

			VerticesDrawn += workflowInstances[i].mesh->getVertexCount();
		}
	}
	ObjectsDrawn = instanceIndex;
}

//draw with the objects pipeline:
void ObjectPipeline::DrawScene(const Scene* scene, const CommandBuffer& commandBuffer)
{
	const auto& allInstances = scene->getObjectInstances();

	//draw all instances in relation to a certain material:
	uint32_t offsetIndex = 0; 
	VertexInput* previouslyBindedVertex = nullptr;

	for (int workflowIndex = 0; workflowIndex < allInstances.size(); ++workflowIndex)
	{
		const auto& workflowInstances = allInstances[workflowIndex];
		if (workflowInstances.empty())
			continue;

		// bind pipeline
		m_MaterialPipelines[workflowIndex]->BindPipeline(commandBuffer);

		// bind descriptor sets
		m_MaterialPipelines[workflowIndex]->BindDescriptors(commandBuffer);

		uint32_t instanceCount = (uint32_t)workflowInstances.size();

		// draw each batch
		for (IndirectBatch& draw : m_IndirectBatches[workflowIndex])
		{
			VertexInput* vertexInputPtr = draw.mesh->getVertexInput();
			if (vertexInputPtr != previouslyBindedVertex) {
				vertexInputPtr->Bind(commandBuffer);
				previouslyBindedVertex = vertexInputPtr;
			}

			draw.mesh->Bind(commandBuffer);
			draw.material->Push(commandBuffer, m_MaterialPipelines[workflowIndex]->m_PipelineLayout);

			constexpr uint32_t stride = sizeof(VkDrawIndexedIndirectCommand);
			VkDeviceSize offset = (draw.firstInstanceIndex + offsetIndex) * stride;

			vkCmdDrawIndexedIndirect(commandBuffer, VulkanContext::Get()->getIndirectBuffer()->getBuffer(), offset, draw.count, stride);
			NumDrawCalls++;
		}
		offsetIndex += instanceCount;
	}
}