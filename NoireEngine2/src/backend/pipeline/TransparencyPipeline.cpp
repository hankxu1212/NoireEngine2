#include "TransparencyPipeline.hpp"

#include "backend/RaytracingContext.hpp"
#include "backend/shader/VulkanShader.h"
#include "backend/shader/ShaderSpecialization.hpp"
#include "backend/pipeline/VulkanGraphicsPipelineBuilder.hpp"

#include "renderer/scene/SceneManager.hpp"
#include "renderer/object/Mesh.hpp"
#include "renderer/materials/Material.hpp"

#include "utils/Logger.hpp"
#include "core/Bitmap.hpp"

#pragma warning (disable:4702)

TransparencyPipeline::~TransparencyPipeline()
{
	m_rtSBTBuffer.Destroy();

	if (m_PipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(VulkanContext::GetDevice(), m_PipelineLayout, nullptr);
		m_PipelineLayout = VK_NULL_HANDLE;
	}

	if (m_Pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(VulkanContext::GetDevice(), m_Pipeline, nullptr);
		m_Pipeline = VK_NULL_HANDLE;
	}
}

void TransparencyPipeline::CreatePipeline()
{
	CreateRayTracingPipeline();
	CreateShaderBindingTables();
}

void TransparencyPipeline::Render(const Scene* scene, const CommandBuffer& commandBuffer)
{
	//m_TransparencyIsDirty |= scene->isSceneDirty;

	//if (m_TransparencyIsDirty)
	//{
	//	m_TransparencyPush.frame = -1;
	//	m_TransparencyIsDirty = false;
	//}
	//m_TransparencyPush.frame++;

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_Pipeline);

	Renderer::Workspace& workspace = Renderer::Instance->workspaces[CURR_FRAME];
	std::vector<VkDescriptorSet> descriptor_sets{
		workspace.set0_World,
		workspace.set1_StorageBuffers,
		Renderer::Instance->set2_Textures,
		Renderer::Instance->set3_IBL,
		Renderer::Instance->set4_ShadowMap,
		Renderer::Instance->set5_RayTracing
	};

	vkCmdBindDescriptorSets(
		commandBuffer, //command buffer
		VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, //pipeline bind point
		m_PipelineLayout, //pipeline layout
		0, //first set
		uint32_t(descriptor_sets.size()), descriptor_sets.data(), //descriptor sets count, ptr
		0, nullptr //dynamic offsets count, ptr
	);

	vkCmdPushConstants(commandBuffer, m_PipelineLayout,
		VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
		0, sizeof(PushConstantRay), &m_TransparencyPush);

	const SwapChain* swapchain = VulkanContext::Get()->getSwapChain(0);
	VkExtent2D extent = swapchain->getExtent();

	RaytracingContext::vkCmdTraceRaysKHR(commandBuffer, &m_rgenRegion, &m_missRegion, &m_hitRegion, &m_callRegion, extent.width, extent.height, 1);
}

void TransparencyPipeline::Prepare(const Scene* scene, const CommandBuffer& commandBuffer)
{
	//if (scene->isSceneDirty)
	//	RaytracingContext::Get()->CreateTopLevelAccelerationStructure(true);
}

void TransparencyPipeline::OnUIRender()
{
	ImGui::SeparatorText("Ray Traced Transparency"); // ---------------------------------
	ImGui::Columns(2);

	// Modify Maximum Samples
	ImGui::Text("Ray Depth");
	ImGui::NextColumn();
	ImGui::DragInt("##RTX_TRANSPARENCY_MAX_DEPTH", &m_TransparencyPush.rayDepth, 1, 1, 10);
	ImGui::Columns(1);
}

void TransparencyPipeline::CreateRayTracingPipeline()
{
	enum StageIndices
	{
		eRaygen,
		eMiss,
		eAnyHit, // <---- 3 specializations of this one
		eShaderGroupCount = 5
	};

	// Specialization
	constexpr uint32_t numSpecializations = 3;
	std::vector<Specialization> specializations(numSpecializations);
	for (int i = 0; i < numSpecializations; i++)
	{
		specializations[i].Add(0, i);
	}

	// load all shaders
	VulkanShader raygenModule("../spv/shaders/raytracing/transparency.rgen.spv", VulkanShader::ShaderStage::RTX_Raygen);
	VulkanShader missModule("../spv/shaders/raytracing/transparency.rmiss.spv", VulkanShader::ShaderStage::RTX_Miss);

	std::array<VkPipelineShaderStageCreateInfo, eShaderGroupCount> stages{};
	VkPipelineShaderStageCreateInfo stage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };

	stages[eRaygen] = raygenModule.shaderStage();
	stages[eMiss] = missModule.shaderStage();

	// Hit Group - Any Hit (no closest hit)
	// Create many variation of the closest hit
	VulkanShader anyHitModule("../spv/shaders/raytracing/transparency.rahit.spv", VulkanShader::ShaderStage::RTX_AHit);
	for (uint32_t s = 0; s < (uint32_t)specializations.size(); s++)
	{
		stages[eAnyHit + s] = anyHitModule.shaderStage();
		stages[eAnyHit + s].pSpecializationInfo = specializations[s].GetSpecialization();
	}

	// Shader groups
	VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
	group.anyHitShader = VK_SHADER_UNUSED_KHR;
	group.closestHitShader = VK_SHADER_UNUSED_KHR;
	group.generalShader = VK_SHADER_UNUSED_KHR;
	group.intersectionShader = VK_SHADER_UNUSED_KHR;

	// Raygen
	group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group.generalShader = eRaygen;
	m_RTShaderGroups.push_back(group);

	// Miss
	group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group.generalShader = eMiss;
	m_RTShaderGroups.push_back(group);

	// Hit Group - AnyHit
	// Creating many Hit groups, one for each specialization
	for (uint32_t s = 0; s < (uint32_t)specializations.size(); s++)
	{
		group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		group.generalShader = VK_SHADER_UNUSED_KHR;
		group.anyHitShader = eAnyHit + s;  // Using variation of the closest hit
		m_RTShaderGroups.push_back(group);
	}

	// Push constant: we want to be able to update constants used by the shaders
	VkPushConstantRange pushConstant{ VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
									 0, sizeof(PushConstantRay) };

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstant;

	std::array< VkDescriptorSetLayout, 6 > layouts{
		Renderer::Instance->set0_WorldLayout,
		Renderer::Instance->set1_StorageBuffersLayout,
		Renderer::Instance->set2_TexturesLayout,
		Renderer::Instance->set3_IBLLayout,
		Renderer::Instance->set4_ShadowMapLayout,
		Renderer::Instance->set5_RayTracingLayout
	};

	pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	pipelineLayoutCreateInfo.pSetLayouts = layouts.data();
	vkCreatePipelineLayout(VulkanContext::GetDevice(), &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout);


	// Assemble the shader stages and recursion depth info into the ray tracing pipeline
	VkRayTracingPipelineCreateInfoKHR rayPipelineInfo{ VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
	rayPipelineInfo.stageCount = static_cast<uint32_t>(stages.size());  // Stages are shaders
	rayPipelineInfo.pStages = stages.data();

	rayPipelineInfo.groupCount = static_cast<uint32_t>(m_RTShaderGroups.size());
	rayPipelineInfo.pGroups = m_RTShaderGroups.data();

	rayPipelineInfo.maxPipelineRayRecursionDepth = 10;  // Ray depth
	rayPipelineInfo.layout = m_PipelineLayout;

	RaytracingContext::vkCreateRayTracingPipelinesKHR(VulkanContext::GetDevice(), {}, {}, 1, &rayPipelineInfo, nullptr, &m_Pipeline);

	NE_DEBUG("Built rtx pipeline", Logger::CYAN, Logger::BOLD);
}

void TransparencyPipeline::CreateShaderBindingTables()
{
	auto& rayTracingPipelineProperties = RaytracingContext::Get()->rayTracingPipelineProperties;

	uint32_t missCount = 1;
	uint32_t hitCount = 3;
	uint32_t handleCount = /*raygen: always 1*/1 + missCount + hitCount;
	uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;

	const uint32_t handleSizeAligned = AlignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);

	m_rgenRegion.stride = AlignedSize(handleSizeAligned, rayTracingPipelineProperties.shaderGroupBaseAlignment);
	m_rgenRegion.size = m_rgenRegion.stride;  // The size member of pRayGenShaderBindingTable must be equal to its stride member

	m_missRegion.stride = handleSizeAligned;
	m_missRegion.size = AlignedSize(missCount * handleSizeAligned, rayTracingPipelineProperties.shaderGroupBaseAlignment);

	m_hitRegion.stride = handleSizeAligned;
	m_hitRegion.size = AlignedSize(hitCount * handleSizeAligned, rayTracingPipelineProperties.shaderGroupBaseAlignment);

	// Get the shader group handles
	uint32_t dataSize = handleCount * handleSize;
	std::vector<uint8_t> handles(dataSize);
	VulkanContext::VK(RaytracingContext::vkGetRayTracingShaderGroupHandlesKHR(VulkanContext::GetDevice(), m_Pipeline, 0, handleCount, dataSize, handles.data()));

	// Allocate a buffer for storing the SBT.
	VkDeviceSize sbtSize = m_rgenRegion.size + m_missRegion.size + m_hitRegion.size + m_callRegion.size;
	m_rtSBTBuffer = Buffer(
		sbtSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		| VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		Buffer::Mapped
	);

	// Find the SBT addresses of each group
	VkDeviceAddress sbtAddress = m_rtSBTBuffer.GetBufferDeviceAddress();
	m_rgenRegion.deviceAddress = sbtAddress;
	m_missRegion.deviceAddress = sbtAddress + m_rgenRegion.size;
	m_hitRegion.deviceAddress = sbtAddress + m_rgenRegion.size + m_missRegion.size;

	// Helper to retrieve the handle data
	auto getHandle = [&](int i) { return handles.data() + i * handleSize; };

	// Map the SBT buffer and write in the handles.
	auto* pSBTBuffer = reinterpret_cast<uint8_t*>(m_rtSBTBuffer.data());
	uint8_t* pData{ nullptr };
	uint32_t handleIdx{ 0 };

	// Raygen
	pData = pSBTBuffer;
	memcpy(pData, getHandle(handleIdx++), handleSize);

	// Miss
	pData = pSBTBuffer + m_rgenRegion.size;
	for (uint32_t c = 0; c < missCount; c++)
	{
		memcpy(pData, getHandle(handleIdx++), handleSize);
		pData += m_missRegion.stride;
	}

	// Hit
	pData = pSBTBuffer + m_rgenRegion.size + m_missRegion.size;
	for (uint32_t c = 0; c < hitCount; c++)
	{
		memcpy(pData, getHandle(handleIdx++), handleSize);
		pData += m_hitRegion.stride;
	}

	m_rtSBTBuffer.UnmapMemory();

	NE_DEBUG("Built RTX SBT", Logger::CYAN, Logger::BOLD);
}