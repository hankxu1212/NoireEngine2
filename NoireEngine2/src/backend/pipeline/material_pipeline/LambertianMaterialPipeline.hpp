#pragma once

#include "MaterialPipeline.hpp"
#include "backend/images/Image2D.hpp"

class LambertianMaterialPipeline : public MaterialPipeline
{
public:
	LambertianMaterialPipeline(ObjectPipeline* objectPipeline);

	void Create() override;
	void BindDescriptors(const CommandBuffer& commandBuffer, Material* materialInstance) override;

private:
	
	void CreatePipelineLayout();
	void CreateGraphicsPipeline();

	VkDescriptorSetLayout			m_DescriptorSetLayoutTexture;
	VkDescriptorSet					m_DescriptorSetTexture;
};

