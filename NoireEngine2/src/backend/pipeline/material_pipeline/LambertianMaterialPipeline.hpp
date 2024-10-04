#pragma once

#include "MaterialPipeline.hpp"
#include "backend/images/Image2D.hpp"

class LambertianMaterialPipeline : public MaterialPipeline
{
public:
	LambertianMaterialPipeline(ObjectPipeline* objectPipeline);

	void Create() override;
	void BindDescriptors(const CommandBuffer& commandBuffer, Material* materialInstance) override;

	glm::vec3						m_Albedo;
	std::shared_ptr<Image2D>		m_AlbedoMap;

	VkDescriptorSetLayout			m_DescriptorSetLayoutTexture;
	VkDescriptorSet					m_DescriptorSetTexture;
};

