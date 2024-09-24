#pragma once

#include "backend/pipeline/ObjectPipeline.hpp"
#include "backend/pipeline/ImGuiPipeline.hpp"
/**
 * A renderer manages the render pass, frame buffers, and swapchain to a certain extent
 * It also manages all pipelines
 */
class Renderer
{
public:
	Renderer();

	void CreatePipelines();

	void CreateRenderPass();

	void Cleanup();

	void Update();

	void Render(const CommandBuffer& commandBuffer, uint32_t surfaceId);

	void Rebuild();

	bool enabled = true;

public:
	std::unique_ptr<ObjectPipeline>			objectPipeline;
	std::unique_ptr<ImGuiPipeline>			imguiPipeline;
};

