#pragma once

#include "MaterialPipeline.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"

class Renderer;

class MirrorMaterialPipeline : public MaterialPipeline
{
public:
	MirrorMaterialPipeline(Renderer* renderer);
	~MirrorMaterialPipeline();

	void Create() override;
	void BindDescriptors(const CommandBuffer& commandBuffer) override;

private:
	void CreateGraphicsPipeline();
	void CreatePipelineLayout();

	DescriptorAllocator						m_DescriptorAllocator;
};

