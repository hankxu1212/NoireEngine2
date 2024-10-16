#pragma once

#include "MaterialPipeline.hpp"
#include "backend/images/Image2D.hpp"

class PBRMaterialPipeline : public MaterialPipeline
{
public:
	PBRMaterialPipeline(ObjectPipeline* objectPipeline);

	void Create() override;
	void BindDescriptors(const CommandBuffer& commandBuffer) override;

private:
	void CreatePipelineLayout();
	void CreateGraphicsPipeline();
};

