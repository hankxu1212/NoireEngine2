#pragma once

#include "VulkanPipeline.hpp"

class LinesPipeline : public VulkanPipeline
{
public:
	LinesPipeline();
	virtual ~LinesPipeline();

	void CreateRenderPass() override;

	void Rebuild() override;

	void CreatePipeline() override;

	void Update(const Scene* scene) override;

	void Render(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId) override;
};
