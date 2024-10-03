#include "MaterialPipeline.hpp"

#include "LambertianMaterialPipeline.hpp"
#include "backend/pipeline/ObjectPipeline.hpp"

MaterialPipeline::MaterialPipeline(ObjectPipeline* objectPipeline) :
	p_ObjectPipeline(objectPipeline) {
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
