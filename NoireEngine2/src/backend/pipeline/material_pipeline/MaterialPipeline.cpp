#include "MaterialPipeline.hpp"

#include "MaterialPipelines.hpp"
#include "backend/pipeline/ObjectPipeline.hpp"
#include "backend/VulkanContext.hpp"
#include "utils/Logger.hpp"

MaterialPipeline::MaterialPipeline(ObjectPipeline* objectPipeline) :
	p_ObjectPipeline(objectPipeline) {
}

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

void MaterialPipeline::BindPipeline(const CommandBuffer& commandBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
}

std::unique_ptr<MaterialPipeline> MaterialPipeline::Create(Material::Workflow workflow, ObjectPipeline* objectPipeline)
{
	switch (workflow)
	{
	case Material::Workflow::Lambertian:
		return std::make_unique<LambertianMaterialPipeline>(objectPipeline);
	case Material::Workflow::Environment:
		return std::make_unique<EnvironmentMaterialPipeline>(objectPipeline);
	case Material::Workflow::Mirror:
		return std::make_unique<MirrorMaterialPipeline>(objectPipeline);
	case Material::Workflow::PBR:
		return std::make_unique<PBRMaterialPipeline>(objectPipeline);
	default:
		NE_ERROR("Did not find a pipeline of this workflow.");
	}
}
