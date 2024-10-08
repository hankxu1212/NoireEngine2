#pragma once

#include "MaterialPipeline.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"
#include "backend/images/ImageCube.hpp"

class ObjectPipeline;

class EnvironmentMaterialPipeline : public MaterialPipeline
{
public:
	EnvironmentMaterialPipeline(ObjectPipeline* objectPipeline);
	~EnvironmentMaterialPipeline();

	void Create() override;
	void BindDescriptors(const CommandBuffer& commandBuffer) override;

private:
	void CreateGraphicsPipeline();
	void CreatePipelineLayout();
	void CreateDescriptors();

	ObjectPipeline*		p_ObjectPipeline;

	VkDescriptorSetLayout set3_CubemapLayout = VK_NULL_HANDLE;
	VkDescriptorSet set3_Cubemap = VK_NULL_HANDLE;

	DescriptorAllocator						m_DescriptorAllocator;
	DescriptorLayoutCache					m_DescriptorLayoutCache;

	std::shared_ptr<ImageCube> cube;
};

