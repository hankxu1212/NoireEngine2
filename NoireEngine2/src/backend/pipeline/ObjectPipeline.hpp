#pragma once

#include "VulkanPipeline.hpp"

class ObjectPipeline : public VulkanPipeline
{
	void CreateShaders() override;
	void CreateDescriptors() override;
	void CreatePipeline(VkRenderPass render_pass, uint32_t subpass) override;
};

