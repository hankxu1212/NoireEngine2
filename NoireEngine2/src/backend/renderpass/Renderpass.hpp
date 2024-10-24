#pragma once

#include "backend/images/ImageDepth.hpp"

class Renderpass
{
public:
	Renderpass(bool hasDepth_);

	~Renderpass();

	void RebuildFromSwapchain();

	void Begin(const CommandBuffer& commandBuffer);

	void End(const CommandBuffer& commandBuffer);

	VkRenderPass							renderpass = VK_NULL_HANDLE;

private:
	void DestroyFrameBuffers();

	std::unique_ptr<ImageDepth>				s_DepthImage;
	std::vector<VkFramebuffer>				m_Framebuffers;
	bool hasDepth;
};

