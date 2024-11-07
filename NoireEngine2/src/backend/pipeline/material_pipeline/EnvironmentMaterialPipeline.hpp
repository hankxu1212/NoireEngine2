#pragma once

#include "MaterialPipeline.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"
#include "backend/images/ImageCube.hpp"

class Renderer;

class EnvironmentMaterialPipeline : public MaterialPipeline
{
public:
	EnvironmentMaterialPipeline(Renderer* renderer);
	~EnvironmentMaterialPipeline();

	void Create() override;
	void BindDescriptors(const CommandBuffer& commandBuffer) override;

private:
	void CreateGraphicsPipeline();
	void CreatePipelineLayout();

	DescriptorAllocator						m_DescriptorAllocator;
};

