#pragma once

#include "MaterialPipeline.hpp"
#include "backend/images/Image2D.hpp"

class PBRMaterialPipeline : public MaterialPipeline
{
public:
	void Create() override;

private:
	void CreatePipelineLayout();
	void CreateGraphicsPipeline();
};

