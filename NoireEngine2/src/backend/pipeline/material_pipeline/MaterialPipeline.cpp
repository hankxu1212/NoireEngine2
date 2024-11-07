#include "MaterialPipeline.hpp"

#include "MaterialPipelines.hpp"
#include "renderer/Renderer.hpp"
#include "backend/VulkanContext.hpp"
#include "utils/Logger.hpp"

MaterialPipeline::~MaterialPipeline()
{
	if (m_PipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(VulkanContext::GetDevice(), m_PipelineLayout, nullptr);
		m_PipelineLayout = VK_NULL_HANDLE;
	}

	if (m_Pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(VulkanContext::GetDevice(), m_Pipeline, nullptr);
		m_Pipeline = VK_NULL_HANDLE;
	}
}

void MaterialPipeline::BindPipeline(const CommandBuffer& commandBuffer) const
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
}

void MaterialPipeline::BindDescriptors(const CommandBuffer& commandBuffer) const
{
	Renderer::Workspace& workspace = Renderer::Instance->workspaces[CURR_FRAME];
	std::array< VkDescriptorSet, 5 > descriptor_sets{
		workspace.set0_World,
		workspace.set1_StorageBuffers,
		Renderer::Instance->set2_Textures,
		Renderer::Instance->set3_Cubemap,
		Renderer::Instance->set4_ShadowMap
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

std::unique_ptr<MaterialPipeline> MaterialPipeline::Create(Material::Workflow workflow)
{
	switch (workflow)
	{
	case Material::Workflow::Lambertian:
		return std::make_unique<LambertianMaterialPipeline>();
	case Material::Workflow::PBR:
		return std::make_unique<PBRMaterialPipeline>();
	default:
		NE_ERROR("Did not find a pipeline of this workflow.");
	}
}
