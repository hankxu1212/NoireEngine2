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

	DescriptorAllocator						m_DescriptorAllocator;
};

