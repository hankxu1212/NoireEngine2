#pragma once

#include "MaterialPipeline.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"

class ObjectPipeline;

class MirrorMaterialPipeline : public MaterialPipeline
{
public:
	MirrorMaterialPipeline(ObjectPipeline* objectPipeline);
	~MirrorMaterialPipeline();

	void Create() override;
	void BindDescriptors(const CommandBuffer& commandBuffer) override;

private:
	void CreateGraphicsPipeline();
	void CreatePipelineLayout();

	ObjectPipeline*		p_ObjectPipeline;

	DescriptorAllocator						m_DescriptorAllocator;
};

