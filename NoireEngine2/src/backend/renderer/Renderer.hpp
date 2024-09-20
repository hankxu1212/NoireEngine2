#pragma once

#include "backend/pipeline/ObjectPipeline.hpp"
#include "backend/images/ImageDepth.hpp"

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

	/**
	 * Runs the render pipeline in the current renderpass.
	 * @param commandBuffer The command buffer to record render command into.
	*/
	void Render(const CommandBuffer& commandBuffer, uint32_t surfaceId);

	void Rebuild();

	bool enabled = true;

private:
	void DestroyFrameBuffers();

public:
	VkRenderPass							m_Renderpass = VK_NULL_HANDLE;
	
	std::unique_ptr<ObjectPipeline>			objectPipeline;
	
	std::unique_ptr<ImageDepth>				s_SwapchainDepthImage;
	std::vector<VkFramebuffer>				m_Framebuffers;

	Scene* scene;
};

