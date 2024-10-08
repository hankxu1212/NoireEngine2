#pragma once

#include "MaterialPipeline.hpp"
#include "backend/images/Image2D.hpp"

class LambertianMaterialPipeline : public MaterialPipeline
{
public:
	LambertianMaterialPipeline(ObjectPipeline* objectPipeline);

	void Create() override;
	void BindDescriptors(const CommandBuffer& commandBuffer) override;

private:
	void CreatePipelineLayout();
	void CreateGraphicsPipeline();
};

