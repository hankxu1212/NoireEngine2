#pragma once

#include "VulkanPipeline.hpp"

class ImGuiPipeline : public VulkanPipeline
{
	ImGuiPipeline(Renderer* renderer);

	virtual ~ImGuiPipeline();

public:
	void CreatePipeline(VkRenderPass& renderpass, uint32_t subpass) override;

	void Render(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId) override;

	void Update(const Scene* scene) override;
};

