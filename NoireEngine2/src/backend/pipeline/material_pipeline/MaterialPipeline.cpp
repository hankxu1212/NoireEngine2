#include "MaterialPipeline.hpp"

#include "LambertianMaterialPipeline.hpp"
#include "backend/pipeline/ObjectPipeline.hpp"
#include "backend/VulkanContext.hpp"

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
	default:
		return std::make_unique<LambertianMaterialPipeline>(objectPipeline); // TODO: change this!
	}
}
