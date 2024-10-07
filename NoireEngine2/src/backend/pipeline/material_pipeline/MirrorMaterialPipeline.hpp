#pragma once

#include "MaterialPipeline.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"
#include "backend/images/ImageCube.hpp"

class ObjectPipeline;

class MirrorMaterialPipeline : public MaterialPipeline
{
public:
	MirrorMaterialPipeline(ObjectPipeline* objectPipeline);
	~MirrorMaterialPipeline();

	void Create() override;
	void BindDescriptors(const CommandBuffer& commandBuffer, uint32_t surfaceId) override;

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

