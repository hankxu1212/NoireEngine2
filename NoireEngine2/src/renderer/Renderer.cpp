#include "Renderer.hpp"

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
#include "renderer/materials/Materials.hpp"

#include "renderer/object/Mesh.hpp"
#include "renderer/scene/Scene.hpp"
#include "renderer/scene/SceneManager.hpp"

#include "backend/pipeline/VulkanGraphicsPipelineBuilder.hpp"

#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, perspective 

#include "renderer/components/CameraComponent.hpp"

#define DEFAULT_SKYBOX "../textures/material_textures/Skybox.png"

static inline void InsertPipelineMemoryBarrier(const CommandBuffer& buf)
{
	VkMemoryBarrier memory_barrier{
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
	};

	// ensures all transfer bit writes are done. before streaming vertices or writing to buffers again
	vkCmdPipelineBarrier(buf,
		VK_PIPELINE_STAGE_TRANSFER_BIT, //srcStageMask
		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, //dstStageMask
		0, //dependencyFlags
		1, &memory_barrier, //memoryBarriers (count, data)
		0, nullptr, //bufferMemoryBarriers (count, data)
		0, nullptr //imageMemoryBarriers (count, data)
	);
}

Renderer::Renderer()
{
	assert(!Instance && "More than one renderer instance found!");
	Instance = this;

	s_OffscreenPass = std::make_unique<Renderpass>();
	s_CompositionPass = std::make_unique<Renderpass>();

	// initialize a bunch of pipelines
	s_UIPipeline = std::make_unique<ImGuiPipeline>();
	s_LinesPipeline = std::make_unique<LinesPipeline>();
	s_SkyboxPipeline = std::make_unique<SkyboxPipeline>();
	s_ShadowPipeline = std::make_unique<ShadowPipeline>();
}

Renderer::~Renderer()
{
	for (Workspace& workspace : workspaces) 
	{
		workspace.TransformsSrc.Destroy();
		workspace.Transforms.Destroy();

		workspace.World.Destroy();
		workspace.WorldSrc.Destroy();
		
		for (int i = 0; i < workspace.Lights.size(); i++)
		{
			workspace.LightsSrc[i].Destroy();
			workspace.Lights[i].Destroy();
		}

		workspace.MaterialInstancesSrc.Destroy();
		workspace.MaterialInstances.Destroy();

		workspace.ObjectDescriptions.Destroy();
		workspace.ObjectDescriptionsSrc.Destroy();
	}
	workspaces.clear();

	m_DescriptorAllocator.Cleanup(); // destroy pool and sets
	
	DestroyFrameBuffers();

	if (m_MaterialPipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(VulkanContext::GetDevice(), m_MaterialPipelineLayout, nullptr);
		m_MaterialPipelineLayout = VK_NULL_HANDLE;
	}

	for (auto& pipeline : m_MaterialPipelines) 
	{
		if (pipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(VulkanContext::GetDevice(), pipeline, nullptr);
			pipeline = VK_NULL_HANDLE;
		}
	}

	if (m_PostPipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(VulkanContext::GetDevice(), m_PostPipelineLayout, nullptr);
		m_PostPipelineLayout = VK_NULL_HANDLE;
	}

	if (m_PostPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(VulkanContext::GetDevice(), m_PostPipeline, nullptr);
		m_PostPipeline = VK_NULL_HANDLE;
	}

	if (m_RaytracedAOComputePipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(VulkanContext::GetDevice(), m_RaytracedAOComputePipeline, nullptr);
		m_RaytracedAOComputePipeline = VK_NULL_HANDLE;
	}

	if (m_RaytracedAOComputePipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(VulkanContext::GetDevice(), m_RaytracedAOComputePipelineLayout, nullptr);
		m_RaytracedAOComputePipelineLayout = VK_NULL_HANDLE;
	}
}

void Renderer::CreateRenderPass()
{
	s_OffscreenPass->CreateRenderPass(
		{ VK_FORMAT_R32G32B32A32_SFLOAT , VK_FORMAT_R32G32B32A32_SFLOAT },  // RGBA + G-Buffer
		VK_FORMAT_D32_SFLOAT_S8_UINT, // todo: actually check
		1, true, true, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL
	);

	s_OffscreenPass->SetClearValues({
		{.color = { { 0.0f, 0.0f, 0.0f, 0.0f } } },  // Clear color to black
		{.color = { { 0.0f, 0.0f, 0.0f, 0.0f } } },  // Clear normal to black
		{.depthStencil = { 1.0f, 0 } }               // Clear depth to 1.0, stencil to 0
	});

	// present to the swapchain
	s_CompositionPass->CreateRenderPass(
		{ VulkanContext::Get()->getSurface(0)->getFormat().format },
		VK_FORMAT_D32_SFLOAT_S8_UINT, // todo: actually check
		1, true, true);

	s_CompositionPass->SetClearValues({
			{.color = { { 0.0f, 0.0f, 0.0f, 0.0f } } },  // Clear color to black
			{.depthStencil = { 1.0f, 0 } }               // Clear depth to 1.0, stencil to 0
	});

	s_UIPipeline->CreateRenderPass();
}

bool showMenu = true;
void Renderer::OnUIRender()
{
	if (ImGui::CollapsingHeader("Renderer Settings", &showMenu))
	{
		ImGui::Checkbox("Use Gizmos", &UseGizmos);
		ImGui::Separator(); // -----------------------------------------------------

		ImGui::Checkbox("Draw Skybox", &DrawSkybox);
		ImGui::Separator(); // -----------------------------------------------------

		// Ray-Traced Ambient Occlusion settings
		ImGui::SeparatorText("Ray Traced Ambient Occlusion"); // ---------------------------------
		ImGui::Columns(2);

		// Modify AO Power
		ImGui::Text("Power");
		ImGui::NextColumn();
		if (ImGui::DragFloat("##RTXAMTOCCLUSION_POWER", &m_AOControl.power, 1.0f, 0.0f, FLT_MAX)) {
			m_AOIsDirty = true;
		}
		ImGui::NextColumn();

		// Modify AO Radius
		ImGui::Text("Radius");
		ImGui::NextColumn();
		if (ImGui::DragFloat("##RTXAMTOCCLUSION_RADIUS", &m_AOControl.radius, 0.1f, 0.0f, FLT_MAX)) {
			m_AOIsDirty = true;
		}
		ImGui::NextColumn();

		// Modify AO Samples
		ImGui::Text("Samples");
		ImGui::NextColumn();
		if (ImGui::DragInt("##RTXAMTOCCLUSION_SAMPLES", &m_AOControl.samples, 1, 1, m_AOControl.maxSamples)) {
			m_AOIsDirty = true;
		}
		ImGui::NextColumn();

		// Modify Distance-Based AO
		ImGui::Text("Distance Based");
		ImGui::NextColumn();
		if (ImGui::Checkbox("##RTXAMTOCCLUSION_DISTANCE_BASED", reinterpret_cast<bool*>(&m_AOControl.distanceBased))) {
			m_AOIsDirty = true;
		}
		ImGui::NextColumn();

		// Modify Maximum Samples
		ImGui::Text("Max Samples");
		ImGui::NextColumn();
		if (ImGui::DragInt("##RTXAMTOCCLUSION_MAX_SAMPLES", &m_AOControl.maxSamples, 1000, 1, INT_MAX)) {
			m_AOIsDirty = true;
		}
		ImGui::Columns(1);

		// rendering stats
		ImGui::SeparatorText("Rendering"); // -----------------------------------------------------

		// num objects drawn
		ImGui::Text("Objects Drawn: %I64u", ObjectsDrawn);
		ImGui::Text("Vertices Drawn: %I64u", VerticesDrawn);
		ImGui::Text("Indirect Indexed Draw Calls: %I64u", NumDrawCalls);
		ImGui::Separator(); // -----------------------------------------------------

		ImGui::BulletText("Application Update Time: %.3fms", Application::ApplicationUpdateTime);
		ImGui::BulletText("Application Render Time: %.3fms", Application::ApplicationRenderTime);
	}
}

void Renderer::CreateMaterialPipelineLayout()
{
	std::vector< VkDescriptorSetLayout> layouts{
		set0_WorldLayout,
		set1_StorageBuffersLayout,
		set2_TexturesLayout,
		set3_IBLLayout,
		set4_ShadowMapLayout,
#ifdef _NE_USE_RTX
		set5_RayTracingLayout
#endif
	};

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = uint32_t(layouts.size()),
		.pSetLayouts = layouts.data(),
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr,
	};

	VulkanContext::VK(vkCreatePipelineLayout(VulkanContext::GetDevice(), &pipelineLayoutCreateInfo, nullptr, &m_MaterialPipelineLayout));
}

void Renderer::CreateMaterialPipelines()
{
	std::vector< VkDynamicState > dynamic_states{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_VERTEX_INPUT_EXT
	};

	std::array< VkPipelineColorBlendAttachmentState, 2 > attachment_states{
		VkPipelineColorBlendAttachmentState{
			.blendEnable = VK_FALSE,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		},
		VkPipelineColorBlendAttachmentState{
			.blendEnable = VK_FALSE,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		},
	};

	VulkanGraphicsPipelineBuilder::Start()
		.SetDynamicStates(dynamic_states)
		.SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.SetColorBlending((uint32_t)attachment_states.size(), attachment_states.data())
		.Build("../spv/shaders/lambertian.vert.spv", "../spv/shaders/lambertian.frag.spv", &m_MaterialPipelines[0], m_MaterialPipelineLayout, s_OffscreenPass->renderpass)
		.Build("../spv/shaders/pbr.vert.spv", "../spv/shaders/pbr.frag.spv", &m_MaterialPipelines[1], m_MaterialPipelineLayout, s_OffscreenPass->renderpass);
}

void Renderer::CreatePostPipelineLayout()
{
	std::vector<VkDescriptorSetLayout> layouts{
		set0_WorldLayout,
		set5_RayTracingLayout
	};

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = uint32_t(layouts.size()),
		.pSetLayouts = layouts.data(),
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr,
	};

	VulkanContext::VK(vkCreatePipelineLayout(VulkanContext::GetDevice(), &pipelineLayoutCreateInfo, nullptr, &m_PostPipelineLayout));
}

void Renderer::CreatePostPipeline()
{
	std::vector< VkDynamicState > dynamic_states{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	std::array< VkPipelineColorBlendAttachmentState, 1 > attachment_states{
		VkPipelineColorBlendAttachmentState{
			.blendEnable = VK_FALSE,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		}
	};

	VkPipelineVertexInputStateCreateInfo vertexInput
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = nullptr,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = nullptr,
	};

	VulkanGraphicsPipelineBuilder::Start()
		.SetDynamicStates(dynamic_states)
		.SetVertexInput(&vertexInput)
		.SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.SetColorBlending((uint32_t)attachment_states.size(), attachment_states.data())
		.SetRasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
		.Build("../spv/shaders/passthrough.vert.spv", "../spv/shaders/post.frag.spv", &m_PostPipeline, m_PostPipelineLayout, s_CompositionPass->renderpass);
}

void Renderer::CreateComputeAOPipeline()
{
	std::vector<VkDescriptorSetLayout> layouts{
		set0_WorldLayout,
		set5_RayTracingLayout,
	};

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = uint32_t(layouts.size()),
		.pSetLayouts = layouts.data(),
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr,
	};

	VkPushConstantRange push_constants = { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(AOPush) };
	VkPipelineLayoutCreateInfo plCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	plCreateInfo.setLayoutCount = uint32_t(layouts.size());
	plCreateInfo.pSetLayouts = layouts.data();
	plCreateInfo.pushConstantRangeCount = 1;
	plCreateInfo.pPushConstantRanges = &push_constants;

	vkCreatePipelineLayout(VulkanContext::GetDevice(), &plCreateInfo, nullptr, &m_RaytracedAOComputePipelineLayout);

	VulkanShader compAOShader("../spv/shaders/raytracing/ao.comp.spv", VulkanShader::ShaderStage::Compute);

	VkComputePipelineCreateInfo cpCreateInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	cpCreateInfo.layout = m_RaytracedAOComputePipelineLayout;
	cpCreateInfo.stage = compAOShader.shaderStage();

	vkCreateComputePipelines(VulkanContext::GetDevice(), {}, 1, &cpCreateInfo, nullptr, &m_RaytracedAOComputePipeline);
}

void Renderer::CreateDescriptors()
{
	CreateWorldDescriptors(false);
	CreateStorageBufferDescriptors();
	CreateTextureDescriptors();
	CreateIBLDescriptors();
	CreateShadowDescriptors();

#ifdef _NE_USE_RTX
	CreateRaytracingDescriptors(false);
#endif

	createdDescriptors = true;
}

void Renderer::CreateWorldDescriptors(bool update)
{
	for (Workspace& workspace : workspaces)
	{
		workspace.World.Destroy();
		workspace.WorldSrc.Destroy();

		workspace.WorldSrc = Buffer(
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

		// build world buffer descriptor
		auto uniformStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT |
			VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

		VkDescriptorImageInfo gBufferColorInfo = workspace.GBufferColors->GetDescriptorInfo();
		VkDescriptorImageInfo gBufferNormalInfo = workspace.GBufferNormals->GetDescriptorInfo();
		VkDescriptorImageInfo rtxAOSamplerInfo = s_RaytracedAOImage->GetDescriptorInfo();

		VkDescriptorBufferInfo World_info = workspace.World.GetDescriptorInfo();
		DescriptorBuilder builder = DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
			.BindBuffer(World::SceneUniform, &World_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformStages)
			.BindImage(World::GBufferColor, &gBufferColorInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT)
			.BindImage(World::GBufferNormal, &gBufferNormalInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

		if (update)
			builder.Write(workspace.set0_World);
		else
			builder.Build(workspace.set0_World, set0_WorldLayout);
	}
}

void Renderer::CreateStorageBufferDescriptors()
{
	// build storage buffers
	for (Workspace& workspace : workspaces)
	{
		DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
			.AddBinding(StorageBuffers::Transform, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT) // transforms
			.AddBinding(StorageBuffers::DirLights, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // dir lights
			.AddBinding(StorageBuffers::SpotLights, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // point lights
			.AddBinding(StorageBuffers::PointLights, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // spot lights
			.AddBinding(StorageBuffers::Materials, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) // material instances
			.AddBinding(StorageBuffers::Objects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) // object descriptions
			.Build(workspace.set1_StorageBuffers, set1_StorageBuffersLayout);
	}
}

void Renderer::CreateTextureDescriptors()
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

	const auto& textures = Resources::Get()->FindAllOfType<Image2D>();

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
	std::vector<VkDescriptorImageInfo> textureDescriptors;
	textureDescriptors.reserve(textures.size());
	for (const auto& tex : textures)
	{
		std::shared_ptr<Image2D> rTex = std::dynamic_pointer_cast<Image2D>(tex);
		textureDescriptors.emplace_back(rTex->GetDescriptorInfo());
	}

	// actually build the descriptor set now
	auto stages = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT;

	DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
		.BindImage(0, textureDescriptors.data(),
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stages, static_cast<uint32_t>(textures.size()))
		.Build(set2_Textures, set2_TexturesLayout, &setLayoutBindingFlags,
#if (defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT))
			// SRS - increase the per-stage descriptor samplers limit on macOS (maxPerStageDescriptorUpdateAfterBindSamplers > maxPerStageDescriptorSamplers)
			VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT
#else
			0 /*VkDescriptorSetLayoutCreateFlags*/
#endif
			, &variableDescriptorInfoAI);
}

void Renderer::CreateIBLDescriptors()
{
	Scene* scene = SceneManager::Get()->getScene();
	if (!scene->m_Skybox)
		scene->AddSkybox(DEFAULT_SKYBOX, Scene::SkyboxType::RGB, true);

	VkDescriptorImageInfo cubeMapInfo = scene->m_Skybox->GetDescriptorInfo();
	VkDescriptorImageInfo lambertianLUTInfo = scene->m_SkyboxLambertian->GetDescriptorInfo();
	VkDescriptorImageInfo specularBRDFInfo = scene->m_SpecularBRDF->GetDescriptorInfo();
	VkDescriptorImageInfo prefilteredEnvMapInfo = scene->m_PrefilteredEnvMap->GetDescriptorInfo();

	auto stages = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
		.BindImage(IBL::Skybox, &cubeMapInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stages | VK_SHADER_STAGE_MISS_BIT_KHR)
		.BindImage(IBL::LambertianLUT, &lambertianLUTInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stages)
		.BindImage(IBL::SpecularBRDF, &specularBRDFInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stages)
		.BindImage(IBL::EnvMap, &prefilteredEnvMapInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stages)
		.Build(set3_IBL, set3_IBLLayout);
}

// create set 4: shadow mapping with descriptor indexing
void Renderer::CreateShadowDescriptors()
{
	const auto& shadowPasses = s_ShadowPipeline->getShadowPasses();
	const auto& cascadePasses = s_ShadowPipeline->getCascadePasses();
	const auto& omniPasses = s_ShadowPipeline->getOmniPasses();

	NE_DEBUG(std::format("Found {} shadow maps.", shadowPasses.size()), Logger::CYAN, Logger::BOLD);
	NE_DEBUG(std::format("Found {} cascaded shadow maps.", cascadePasses.size()), Logger::CYAN, Logger::BOLD);
	NE_DEBUG(std::format("Found {} omnidirectional shadow maps.", omniPasses.size()), Logger::CYAN, Logger::BOLD);

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

void Renderer::CreateRayTracingImages()
{
	const SwapChain* swapchain = VulkanContext::Get()->getSwapChain();
	glm::uvec2 extent = swapchain->getExtentVec2();

	// ray traced ambient occlusion result (r32)
	{
		s_RaytracedAOImage = std::make_unique<Image2D>(
			extent.x, extent.y,
			VK_FORMAT_R32_SFLOAT,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			false
		);

		s_RaytracedAOImage->Load();
	}

	// ray traced reflection (rgba32f)
	{
		s_RaytracedReflectionsImage = std::make_unique<Image2D>(
			extent.x, extent.y,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			false
		);

		s_RaytracedReflectionsImage->Load();
	}
}

void Renderer::CreateRaytracingDescriptors(bool update)
{
	// create the descriptors
	auto as = s_RaytracingPipeline->GetTLAS();

	VkWriteDescriptorSetAccelerationStructureKHR descASInfo{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
	descASInfo.accelerationStructureCount = 1;
	descASInfo.pAccelerationStructures = &as;

	VkDescriptorImageInfo reflectImageInfo = s_RaytracedReflectionsImage->GetDescriptorInfo();
	VkDescriptorImageInfo aoImageInfo = s_RaytracedAOImage->GetDescriptorInfo();

	DescriptorBuilder builder = DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
		.BindAccelerationStructure(RTXBindings::TLAS, descASInfo, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(RTXBindings::ReflectionImage, &reflectImageInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT)
		.BindImage(RTXBindings::AOImage, &aoImageInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

	if (update)
		builder.Write(set5_RayTracing);
	else
		builder.Build(set5_RayTracing, set5_RayTracingLayout);
}

void Renderer::Rebuild()
{
	workspaces.resize(VulkanContext::Get()->getFramesInFlight());

	DestroyFrameBuffers();
	CreateRayTracingImages();
	CreateFrameBuffers();

	if (createdDescriptors) 
	{
#ifdef _NE_USE_RTX
		CreateRaytracingDescriptors(true);
#endif
		CreateWorldDescriptors(true);
	}

	s_UIPipeline->Rebuild();
	if (s_RaytracedAOImage.get() != nullptr && createdDescriptors)
		s_UIPipeline->SetupDebugViewport(s_RaytracedAOImage.get());

	m_AOIsDirty = true;
}

void Renderer::Create()
{
	// create shadow pipeline and initialize its shadow frame buffer and render pass
	s_ShadowPipeline->CreateRenderPass();

#ifdef _NE_USE_RTX
	s_RaytracingPipeline = std::make_unique<RaytracingPipeline>();
	s_RaytracingPipeline->CreateAccelerationStructures();
#endif

	// this relies on shadow pipeline's depth attachment and ray tracing's acceleration structures
	CreateDescriptors();
	
	CreateMaterialPipelineLayout();
	CreateMaterialPipelines();

	CreatePostPipelineLayout();
	CreatePostPipeline();

	CreateComputeAOPipeline();

	// the following pipelines rely on Renderer's descriptor sets
	s_ShadowPipeline->CreatePipeline();

	s_LinesPipeline->CreatePipeline();

	s_SkyboxPipeline->CreatePipeline();

	s_UIPipeline->CreatePipeline();

#ifdef _NE_USE_RTX
	s_RaytracingPipeline->CreatePipeline();
	s_UIPipeline->SetupDebugViewport(s_RaytracedAOImage.get());
#endif
}

void Renderer::Render(const CommandBuffer& commandBuffer)
{
	const Scene* scene = SceneManager::Get()->getScene();
	
	// prepare and upload/update info and buffers
	{
		Prepare(scene, commandBuffer);

		s_ShadowPipeline->Prepare(scene, commandBuffer);

		if (UseGizmos)
			s_LinesPipeline->Prepare(scene, commandBuffer);
		
		if (DrawSkybox)
			s_SkyboxPipeline->Prepare(scene, commandBuffer);
	}
	
	//memory barrier to make sure copies complete before rendering happens:
	InsertPipelineMemoryBarrier(commandBuffer);

	// render objects
	{
		// draw rtx
#ifdef _NE_USE_RTX
		RunRTXReflection(scene, commandBuffer);
#endif

		// render shadow passes
		s_ShadowPipeline->Render(scene, commandBuffer);

		s_OffscreenPass->Begin(commandBuffer, m_OffscreenFrameBuffers[CURR_FRAME]);
		{
			DrawScene(scene, commandBuffer);

			// draw skybox
			if (DrawSkybox)
				s_SkyboxPipeline->Render(scene, commandBuffer);
		}
		s_OffscreenPass->End(commandBuffer);

		RunAOCompute(commandBuffer);

		s_CompositionPass->Begin(commandBuffer, m_CompositionFrameBuffers[CURR_FRAME]);
		{
			RunPost(commandBuffer);

			// draw lines: TODO: move to ui render pass
			if (UseGizmos)
				s_LinesPipeline->Render(scene, commandBuffer);
		}
		s_CompositionPass->End(commandBuffer);

		s_UIPipeline->Render(scene, commandBuffer);
	}
}

void Renderer::Update()
{
	s_UIPipeline->Update(SceneManager::Get()->getScene());
}

void Renderer::Prepare(const Scene* scene, const CommandBuffer& commandBuffer)
{
	PrepareSceneUniform(scene, commandBuffer);
	PrepareTransforms(scene, commandBuffer);
	PrepareLights(scene, commandBuffer);
	PrepareMaterialInstances(commandBuffer);
	PrepareObjectDescriptions(scene, commandBuffer);
	PrepareIndirectDrawBuffer(scene);
}

void Renderer::PrepareSceneUniform(const Scene* scene, const CommandBuffer& commandBuffer)
{
	Workspace& workspace = workspaces[CURR_FRAME];
	Buffer::CopyFromHost(commandBuffer, workspace.WorldSrc, workspace.World, sizeof(Scene::SceneUniform), scene->getSceneUniformPtr());
}

void Renderer::PrepareTransforms(const Scene* scene, const CommandBuffer& commandBuffer)
{
	Workspace& workspace = workspaces[CURR_FRAME];

	const auto& sceneObjectInstances = scene->getObjectInstances();

	size_t needed_bytes = 0;
	for (int i = 0; i < sceneObjectInstances.size(); ++i)
		needed_bytes += sceneObjectInstances[i].size() * sizeof(ObjectInstance::TransformUniform);

	// resize as neccesary
	if (workspace.TransformsSrc.getBuffer() == VK_NULL_HANDLE
		|| workspace.TransformsSrc.getSize() < needed_bytes)
	{
		//round to next multiple of 4k to avoid re-allocating continuously if vertex count grows slowly:
		size_t new_bytes = ((needed_bytes + 4096) / 4096) * 4096;

		workspace.TransformsSrc.Destroy();
		workspace.Transforms.Destroy();

		workspace.TransformsSrc = Buffer(
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

	assert(workspace.TransformsSrc.getSize() == workspace.Transforms.getSize());
	assert(workspace.TransformsSrc.getSize() >= needed_bytes);

	{ //copy transforms into Transforms_src:
		ObjectInstance::TransformUniform* start = reinterpret_cast<ObjectInstance::TransformUniform*>(workspace.TransformsSrc.data());
		ObjectInstance::TransformUniform* out = start;

		for (int i = 0; i < sceneObjectInstances.size(); ++i) {
			for (auto& inst : sceneObjectInstances[i]) {
				*out = inst.m_TransformUniform;
				++out;
			}
		}

		size_t regionSize = reinterpret_cast<char*>(out) - reinterpret_cast<char*>(start);
		Buffer::CopyBuffer(commandBuffer, workspace.TransformsSrc.getBuffer(), workspace.Transforms.getBuffer(), regionSize);
	}
}

void Renderer::PrepareLights(const Scene* scene, const CommandBuffer& commandBuffer)
{
	Workspace& workspace = workspaces[CURR_FRAME];

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
		Buffer& bufSrc = workspace.LightsSrc[i];
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
			VkDescriptorBufferInfo LightInfo = buf.GetDescriptorInfo();
			DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
				.BindBuffer(i + 1 /*cuz index 0 is transforms*/, &LightInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.Write(workspace.set1_StorageBuffers);

			NE_INFO("Reallocated light type {} to {} bytes", i, new_bytes);
		}

		assert(bufSrc.getSize() == buf.getSize());
		assert(bufSrc.getSize() >= neededBytes);
	}

	// Get pointers to the start of each region
	DirectionalLightUniform* directionalStart = reinterpret_cast<DirectionalLightUniform*>(workspace.LightsSrc[0].data());
	PointLightUniform* pointStart = reinterpret_cast<PointLightUniform*>(workspace.LightsSrc[1].data());
	SpotLightUniform* spotStart = reinterpret_cast<SpotLightUniform*>(workspace.LightsSrc[2].data());

	// Pointers to track current positions
	DirectionalLightUniform* directionalIt = directionalStart;
	PointLightUniform* pointIt = pointStart;
	SpotLightUniform* spotIt = spotStart;

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

	// Calculate the sizes for each buffer
	size_t directionalSize = reinterpret_cast<char*>(directionalIt) - reinterpret_cast<char*>(directionalStart);
	size_t pointSize = reinterpret_cast<char*>(pointIt) - reinterpret_cast<char*>(pointStart);
	size_t spotSize = reinterpret_cast<char*>(spotIt) - reinterpret_cast<char*>(spotStart);

	Buffer::CopyBuffer(commandBuffer, workspace.LightsSrc[0].getBuffer(), workspace.Lights[0].getBuffer(), directionalSize);
	Buffer::CopyBuffer(commandBuffer, workspace.LightsSrc[1].getBuffer(), workspace.Lights[1].getBuffer(), pointSize);
	Buffer::CopyBuffer(commandBuffer, workspace.LightsSrc[2].getBuffer(), workspace.Lights[2].getBuffer(), spotSize);
}

void Renderer::PrepareObjectDescriptions(const Scene* scene, const CommandBuffer& commandBuffer)
{
	Workspace& workspace = workspaces[CURR_FRAME];

	const auto& sceneObjectInstances = scene->getObjectInstances();

	size_t needed_bytes = 0;
	for (int i = 0; i < sceneObjectInstances.size(); ++i)
		needed_bytes += sceneObjectInstances[i].size() * sizeof(ObjectDescription);

	// resize as neccesary
	if (workspace.ObjectDescriptionsSrc.getBuffer() == VK_NULL_HANDLE
		|| workspace.ObjectDescriptionsSrc.getSize() < needed_bytes)
	{
		//round to next multiple of 4k to avoid re-allocating continuously if vertex count grows slowly:
		size_t new_bytes = ((needed_bytes + 4096) / 4096) * 4096;

		workspace.ObjectDescriptionsSrc.Destroy();
		workspace.ObjectDescriptions.Destroy();

		workspace.ObjectDescriptionsSrc = Buffer(
			new_bytes,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			Buffer::Mapped
		);
		workspace.ObjectDescriptions = Buffer(
			new_bytes,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		//update the descriptor set:
		VkDescriptorBufferInfo objBufferInfo = workspace.ObjectDescriptions.GetDescriptorInfo();
		DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
			.BindBuffer(StorageBuffers::Objects, &objBufferInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.Write(workspace.set1_StorageBuffers);

		NE_INFO("Reallocated object descriptions to {} bytes", new_bytes);
	}

	assert(workspace.ObjectDescriptionsSrc.getSize() == workspace.ObjectDescriptions.getSize());
	assert(workspace.ObjectDescriptionsSrc.getSize() >= needed_bytes);

	{
		ObjectDescription* start = reinterpret_cast<ObjectDescription*>(workspace.ObjectDescriptionsSrc.data());
		ObjectDescription* out = start;

		for (int i = 0; i < sceneObjectInstances.size(); ++i) 
		{
			for (auto& inst : sceneObjectInstances[i]) 
			{
				ObjectDescription description
				{
					.vertexAddress = inst.mesh->getVertexBufferAddress(),
					.indexAddress = inst.mesh->getIndexBufferAddress(),
					.materialOffset = (uint32_t)inst.material->materialInstanceBufferOffset,
					.materialWorkflow = (uint32_t)inst.material->getWorkflow()
				};
				*out = description;
				++out;
			}
		}

		size_t regionSize = reinterpret_cast<char*>(out) - reinterpret_cast<char*>(start);
		Buffer::CopyBuffer(commandBuffer, workspace.ObjectDescriptionsSrc.getBuffer(), workspace.ObjectDescriptions.getBuffer(), regionSize);
	}
}

void Renderer::PrepareMaterialInstances(const CommandBuffer& commandBuffer)
{
	Workspace& workspace = workspaces[CURR_FRAME];

	size_t neededBytes = 0;
	const auto& materialInstances = Resources::Get()->FindAllOfType<Material>();
	for (const auto& resource : materialInstances)
	{
		std::shared_ptr<Material> mat = std::dynamic_pointer_cast<Material>(resource);

		switch (mat->getWorkflow())
		{
		case Material::Workflow::Lambertian:
			neededBytes += sizeof(LambertianMaterial::MaterialUniform);
			break;
		case Material::Workflow::PBR:
			neededBytes += sizeof(PBRMaterial::MaterialUniform);
			break;
		default:
			break;
		}
	}

	// resize as neccesary
	if (workspace.MaterialInstancesSrc.getBuffer() == VK_NULL_HANDLE
		|| workspace.MaterialInstancesSrc.getSize() < neededBytes)
	{
		size_t new_bytes = ((neededBytes + 4096) / 4096) * 4096;

		workspace.MaterialInstancesSrc.Destroy();
		workspace.MaterialInstances.Destroy();

		workspace.MaterialInstancesSrc = Buffer(
			new_bytes,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			Buffer::Mapped
		);
		workspace.MaterialInstances = Buffer(
			new_bytes,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		//update the descriptor set:
		VkDescriptorBufferInfo matInfo = workspace.MaterialInstances.GetDescriptorInfo();
		DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
			.BindBuffer(StorageBuffers::Materials, &matInfo, 
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
				VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
			.Write(workspace.set1_StorageBuffers);

		NE_INFO("Reallocated material instances to {} bytes", new_bytes);
	}

	assert(workspace.MaterialInstancesSrc.getSize() == workspace.MaterialInstances.getSize());
	assert(workspace.MaterialInstancesSrc.getSize() >= neededBytes);

	void* start = workspace.MaterialInstancesSrc.data();
	void* out = start;

	for (const auto& resource : materialInstances)
	{
		std::shared_ptr<Material> mat = std::dynamic_pointer_cast<Material>(resource);
		mat->materialInstanceBufferOffset = CALCULATE_OFFSET(start, out) / sizeof(float);
		switch (mat->getWorkflow())
		{
		case Material::Workflow::Lambertian:
			memcpy(out, mat->getPushPointer(), sizeof(LambertianMaterial::MaterialUniform));
			out = PTR_ADD(out, sizeof(LambertianMaterial::MaterialUniform));
			break;
		case Material::Workflow::PBR:
			memcpy(out, mat->getPushPointer(), sizeof(PBRMaterial::MaterialUniform));
			out = PTR_ADD(out, sizeof(PBRMaterial::MaterialUniform));
			break;
		}
	}
	Buffer::CopyBuffer(commandBuffer, workspace.MaterialInstancesSrc.getBuffer(), workspace.MaterialInstances.getBuffer(), neededBytes);
}

void Renderer::CompactDraws(const std::vector<ObjectInstance>& objects, uint32_t workflowIndex)
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

void Renderer::PrepareIndirectDrawBuffer(const Scene* scene)
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

void Renderer::DrawScene(const Scene* scene, const CommandBuffer& commandBuffer)
{
	Workspace& workspace = workspaces[CURR_FRAME];
	const auto& allInstances = scene->getObjectInstances();

	//draw all instances in relation to a certain material:
	uint32_t offsetIndex = 0; 
	VertexInput* previouslyBindedVertex = nullptr;

	// bind descriptor sets
	std::vector<VkDescriptorSet> descriptor_sets{
		workspace.set0_World,
		workspace.set1_StorageBuffers,
		set2_Textures,
		set3_IBL,
		set4_ShadowMap,
#ifdef _NE_USE_RTX
		set5_RayTracing
#endif
	};

	vkCmdBindDescriptorSets(
		commandBuffer, //command buffer
		VK_PIPELINE_BIND_POINT_GRAPHICS, //pipeline bind point
		m_MaterialPipelineLayout, //pipeline layout
		0, //first set
		uint32_t(descriptor_sets.size()), descriptor_sets.data(), //descriptor sets count, ptr
		0, nullptr //dynamic offsets count, ptr
	);

	for (int workflowIndex = 0; workflowIndex < allInstances.size(); ++workflowIndex)
	{
		const auto& workflowInstances = allInstances[workflowIndex];
		if (workflowInstances.empty())
			continue;

		// bind pipeline
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_MaterialPipelines[workflowIndex]);

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

			constexpr uint32_t stride = sizeof(VkDrawIndexedIndirectCommand);
			VkDeviceSize offset = (draw.firstInstanceIndex + offsetIndex) * stride;

			vkCmdDrawIndexedIndirect(commandBuffer, VulkanContext::Get()->getIndirectBuffer()->getBuffer(), offset, draw.count, stride);
			NumDrawCalls++;
		}
		offsetIndex += instanceCount;
	}
}

void Renderer::RunAOCompute(const CommandBuffer& commandBuffer)
{
	m_AOIsDirty |= VIEW_CAM->wasDirtyThisFrame;

	if (m_AOIsDirty) 
	{
		m_AOControl.frame = -1;
		m_AOIsDirty = false;
	}
	m_AOControl.frame++;

	// Adding a barrier to be sure the fragment has finished writing to the G-Buffer
	// before the compute shader is using the buffer
	Image::InsertImageMemoryBarrier(commandBuffer, workspaces[CURR_FRAME].GBufferNormals->getImage(),
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		1, 0, 1, 0);

	// Preparing for the compute shader
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_RaytracedAOComputePipeline);

	auto& workspace = workspaces[CURR_FRAME];

	// bind descriptor sets
	std::vector<VkDescriptorSet> descriptor_sets{
		workspace.set0_World,
#ifdef _NE_USE_RTX
		set5_RayTracing
#endif
	};

	vkCmdBindDescriptorSets(
		commandBuffer, //command buffer
		VK_PIPELINE_BIND_POINT_COMPUTE, //pipeline bind point
		m_RaytracedAOComputePipelineLayout, //pipeline layout
		0, //first set
		uint32_t(descriptor_sets.size()), descriptor_sets.data(), //descriptor sets count, ptr
		0, nullptr //dynamic offsets count, ptr
	);

	// Sending the push constant information
	vkCmdPushConstants(commandBuffer, m_RaytracedAOComputePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(AOPush), &m_AOControl);

	// Dispatching the compute shader
	constexpr uint32_t GROUP_SIZE = 16;
	const SwapChain* swapchain = VulkanContext::Get()->getSwapChain(0);
	VkExtent2D extent = swapchain->getExtent();
	vkCmdDispatch(commandBuffer, (extent.width + (GROUP_SIZE - 1)) / GROUP_SIZE, (extent.height + (GROUP_SIZE - 1)) / GROUP_SIZE, 1);

	// Adding a barrier to be sure the compute shader has finished
	// writing to the AO buffer before the post shader is using it
	Image::InsertImageMemoryBarrier(commandBuffer, s_RaytracedAOImage->getImage(),
		VK_ACCESS_SHADER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		1, 0, 1, 0);
}

void Renderer::RunRTXReflection(const Scene* scene, const CommandBuffer& commandBuffer)
{
	s_RaytracingPipeline->Render(scene, commandBuffer);

	// Adding a barrier to be sure the ray tracing pipeline shader has finished
	// writing to the reflection image before fragment shader reads from it
	Image::InsertImageMemoryBarrier(commandBuffer, s_RaytracedAOImage->getImage(),
		VK_ACCESS_SHADER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		1, 0, 1, 0);
}

void Renderer::RunPost(const CommandBuffer& commandBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PostPipeline);

	auto& workspace = workspaces[CURR_FRAME];

	// bind descriptor sets
	std::vector<VkDescriptorSet> descriptor_sets{
		workspace.set0_World,
#ifdef _NE_USE_RTX
		set5_RayTracing
#endif
	};

	vkCmdBindDescriptorSets(
		commandBuffer, //command buffer
		VK_PIPELINE_BIND_POINT_GRAPHICS, //pipeline bind point
		m_PostPipelineLayout, //pipeline layout
		0, //first set
		uint32_t(descriptor_sets.size()), descriptor_sets.data(), //descriptor sets count, ptr
		0, nullptr //dynamic offsets count, ptr
	);

	// draw full screen quad
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void Renderer::DestroyFrameBuffers()
{
	for (VkFramebuffer& framebuffer : m_CompositionFrameBuffers)
	{
		if (framebuffer != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(VulkanContext::GetDevice(), framebuffer, nullptr);
			framebuffer = VK_NULL_HANDLE;
		}
	}
	m_CompositionFrameBuffers.clear();

	for (VkFramebuffer& framebuffer : m_OffscreenFrameBuffers)
	{
		if (framebuffer != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(VulkanContext::GetDevice(), framebuffer, nullptr);
			framebuffer = VK_NULL_HANDLE;
		}
	}
	m_OffscreenFrameBuffers.clear();
}

void Renderer::CreateFrameBuffers()
{
	const SwapChain* swapchain = VulkanContext::Get()->getSwapChain();
	glm::uvec2 extent = swapchain->getExtentVec2();

	uint32_t imageCnt = swapchain->getImageCount();
	m_CompositionFrameBuffers.resize(imageCnt);
	m_OffscreenFrameBuffers.resize(imageCnt);

	s_MainDepth = std::make_unique<ImageDepth>(extent);


	for (uint32_t i = 0; i < imageCnt; ++i)
	{
		// G-Buffer color (rgba32f)
		{
			workspaces[i].GBufferColors = std::make_unique<Image2D>(
				extent.x, extent.y,
				VK_FORMAT_R32G32B32A32_SFLOAT,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
				false
			);

			workspaces[i].GBufferColors->Load(nullptr, false);
		}

		// G-Buffer normals (rgba32f) - position(xyz) / normal(w-compressed)
		{
			workspaces[i].GBufferNormals = std::make_unique<Image2D>(
				extent.x, extent.y,
				VK_FORMAT_R32G32B32A32_SFLOAT,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
				false
			);

			workspaces[i].GBufferNormals->Load(nullptr, false);
		}

		// create all swapchain frame buffers
		{
			std::vector<VkImageView> swapchainAttachments{
				swapchain->getImageViews()[i],
				s_MainDepth->getView()
			};

			VkFramebufferCreateInfo create_info
			{
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = s_CompositionPass->renderpass,
				.attachmentCount = uint32_t(swapchainAttachments.size()),
				.pAttachments = swapchainAttachments.data(),
				.width = swapchain->getExtent().width,
				.height = swapchain->getExtent().height,
				.layers = 1,
			};

			VulkanContext::VK(vkCreateFramebuffer(VulkanContext::GetDevice(), &create_info, nullptr, &m_CompositionFrameBuffers[i]));
		}

		// create all offscreen frame buffers
		{
			std::vector<VkImageView> offscreenAttachments{
				workspaces[i].GBufferColors->getView(),
				workspaces[i].GBufferNormals->getView(),
				s_MainDepth->getView()
			};

			VkFramebufferCreateInfo create_info
			{
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = s_OffscreenPass->renderpass,
				.attachmentCount = uint32_t(offscreenAttachments.size()),
				.pAttachments = offscreenAttachments.data(),
				.width = swapchain->getExtent().width,
				.height = swapchain->getExtent().height,
				.layers = 1,
			};

			VulkanContext::VK(vkCreateFramebuffer(VulkanContext::GetDevice(), &create_info, nullptr, &m_OffscreenFrameBuffers[i]));
		}
	}
}
