#pragma once

#include "backend/images/ImageDepth.hpp"

struct Renderpass
{
	~Renderpass();

	void CreateRenderPass(
		const std::vector<VkFormat>& colorAttachmentFormats,
		VkFormat                     depthAttachmentFormat,
		uint32_t                     subpassCount = 1,
		bool                         clearColor = true,
		bool                         clearDepth = true,
		VkImageLayout                initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkImageLayout                finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	
	void SetClearValues(const std::vector<VkClearValue>& values);

	void Begin(const CommandBuffer& commandBuffer, VkFramebuffer fb);

	void Begin(const CommandBuffer& commandBuffer, VkFramebuffer fb, VkExtent2D extent);

	void End(const CommandBuffer& commandBuffer);

	VkRenderPass renderpass = VK_NULL_HANDLE;
	std::vector<VkClearValue> clearValues;
};

