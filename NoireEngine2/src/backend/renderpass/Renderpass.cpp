#include "Renderpass.hpp"
#include "backend/VulkanContext.hpp"

Renderpass::Renderpass(bool hasDepth_) :
	hasDepth(hasDepth_)
{
}

Renderpass::~Renderpass()
{
	DestroyFrameBuffers();

	if (renderpass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(VulkanContext::GetDevice(), renderpass, nullptr);
	}
	renderpass = VK_NULL_HANDLE;
}

void Renderpass::Rebuild()
{
	DestroyFrameBuffers();

	// TODO: add support for multiple swapchains
	const SwapChain* swapchain = VulkanContext::Get()->getSwapChain(0);

	if (hasDepth)
		s_SwapchainDepthImage = std::make_unique<ImageDepth>(swapchain->getExtentVec2(), VK_SAMPLE_COUNT_1_BIT);

	//Make framebuffers for each swapchain image:
	m_Framebuffers.assign(swapchain->getImageViews().size(), VK_NULL_HANDLE);
	for (size_t i = 0; i < swapchain->getImageViews().size(); ++i) 
	{
		std::vector<VkImageView> attachments;
		attachments.emplace_back(swapchain->getImageViews()[i]);
		if (hasDepth)
			attachments.emplace_back(s_SwapchainDepthImage->getView());

		VkFramebufferCreateInfo create_info
		{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = renderpass,
			.attachmentCount = uint32_t(attachments.size()),
			.pAttachments = attachments.data(),
			.width = swapchain->getExtent().width,
			.height = swapchain->getExtent().height,
			.layers = 1,
		};

		VulkanContext::VK_CHECK(
			vkCreateFramebuffer(VulkanContext::GetDevice(), &create_info, nullptr, &m_Framebuffers[i]),
			"[vulkan] Creating frame buffer failed"
		);
	}
}

void Renderpass::Begin(const CommandBuffer& commandBuffer)
{
	VkExtent2D swapChainExtent = VulkanContext::Get()->getSwapChain()->getExtent();

	static std::array< VkClearValue, 2 > clear_values{
		VkClearValue{.color{.float32{0.2f, 0.2f, 0.2f, 0.2f} } },
		VkClearValue{.depthStencil{.depth = 1.0f, .stencil = 0 } },
	};

	VkRenderPassBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderpass,
		.framebuffer = m_Framebuffers[VulkanContext::Get()->getCurrentFrame()],
		.renderArea{
			.offset = {.x = 0, .y = 0},
			.extent = swapChainExtent,
		},
		.clearValueCount = uint32_t(clear_values.size()),
		.pClearValues = clear_values.data(),
	};

	vkCmdBeginRenderPass(commandBuffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

	VkRect2D scissor{
		.offset = {.x = 0, .y = 0},
		.extent = swapChainExtent,
	};
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	VkViewport viewport{
		.x = 0.0f,
		.y = 0.0f,
		.width = float(swapChainExtent.width),
		.height = float(swapChainExtent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
}

void Renderpass::End(const CommandBuffer& commandBuffer)
{
	vkCmdEndRenderPass(commandBuffer);
}

void Renderpass::DestroyFrameBuffers()
{
	for (VkFramebuffer& framebuffer : m_Framebuffers)
	{
		if(framebuffer != VK_NULL_HANDLE)
			vkDestroyFramebuffer(VulkanContext::GetDevice(), framebuffer, nullptr);
		framebuffer = VK_NULL_HANDLE;
	}
	m_Framebuffers.clear();
	s_SwapchainDepthImage.reset();
}
