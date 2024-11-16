#include "Renderpass.hpp"
#include "backend/VulkanContext.hpp"

Renderpass::~Renderpass()
{
	if (renderpass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(VulkanContext::GetDevice(), renderpass, nullptr);
		renderpass = VK_NULL_HANDLE;
	}
}

void Renderpass::CreateRenderPass(
	const std::vector<VkFormat>& colorAttachmentFormats, 
	VkFormat depthAttachmentFormat, 
	uint32_t subpassCount, 
	bool clearColor, 
	bool clearDepth, 
	VkImageLayout initialLayout, 
	VkImageLayout finalLayout,
	MSAAInfo* pMsaaInfo)
{
	std::vector<VkAttachmentDescription> allAttachments;
	std::vector<VkAttachmentReference>   colorAttachmentRefs;

	bool hasDepth = (depthAttachmentFormat != VK_FORMAT_UNDEFINED);
	
	VkSampleCountFlagBits samples = pMsaaInfo ? pMsaaInfo->samples : VK_SAMPLE_COUNT_1_BIT;

	for (const auto& format : colorAttachmentFormats)
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = format;
		colorAttachment.samples = samples;
		colorAttachment.loadOp = clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR :
			((initialLayout == VK_IMAGE_LAYOUT_UNDEFINED) ? VK_ATTACHMENT_LOAD_OP_DONT_CARE :
				VK_ATTACHMENT_LOAD_OP_LOAD);
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = initialLayout;
		colorAttachment.finalLayout = finalLayout;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = static_cast<uint32_t>(allAttachments.size());
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		allAttachments.push_back(colorAttachment);
		colorAttachmentRefs.push_back(colorAttachmentRef);
	}

	VkAttachmentReference depthAttachmentRef{};
	if (hasDepth)
	{
		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = depthAttachmentFormat;
		depthAttachment.samples = samples;
		depthAttachment.loadOp = clearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;

		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		depthAttachmentRef.attachment = static_cast<uint32_t>(allAttachments.size());
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		allAttachments.push_back(depthAttachment);
	}

	VkAttachmentReference msaaAttachmentRef{};
	if (pMsaaInfo)
	{
		VkAttachmentDescription colorAttachmentResolve{};
		colorAttachmentResolve.format = pMsaaInfo->format;
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		msaaAttachmentRef.attachment = static_cast<uint32_t>(allAttachments.size());
		msaaAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		allAttachments.push_back(colorAttachmentResolve);
	}

	std::vector<VkSubpassDescription> subpasses;
	std::vector<VkSubpassDependency>  subpassDependencies;

	for (uint32_t i = 0; i < subpassCount; i++)
	{
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
		subpass.pColorAttachments = colorAttachmentRefs.data();
		subpass.pDepthStencilAttachment = hasDepth ? &depthAttachmentRef : VK_NULL_HANDLE;
		subpass.pResolveAttachments = pMsaaInfo ? &msaaAttachmentRef : VK_NULL_HANDLE;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = i == 0 ? (VK_SUBPASS_EXTERNAL) : (i - 1);
		dependency.dstSubpass = i;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		subpasses.push_back(subpass);
		subpassDependencies.push_back(dependency);
	}

	VkRenderPassCreateInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassInfo.attachmentCount = static_cast<uint32_t>(allAttachments.size());
	renderPassInfo.pAttachments = allAttachments.data();
	renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
	renderPassInfo.pSubpasses = subpasses.data();
	renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	renderPassInfo.pDependencies = subpassDependencies.data();

	VulkanContext::VK(vkCreateRenderPass(VulkanContext::GetDevice(), &renderPassInfo, nullptr, &renderpass));
}

void Renderpass::SetClearValues(const std::vector<VkClearValue>& values)
{
	clearValues = values;
}

void Renderpass::Begin(const CommandBuffer& commandBuffer, VkFramebuffer fb)
{
	VkExtent2D swapChainExtent = VulkanContext::Get()->getSwapChain()->getExtent();

	VkRenderPassBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderpass,
		.framebuffer = fb,
		.renderArea{
			.offset = {.x = 0, .y = 0},
			.extent = swapChainExtent,
		},
		.clearValueCount = uint32_t(clearValues.size()),
		.pClearValues = clearValues.data(),
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

void Renderpass::Begin(const CommandBuffer& commandBuffer, VkFramebuffer fb, VkExtent2D extent)
{
	VkRenderPassBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderpass,
		.framebuffer = fb,
		.renderArea{
			.offset = {.x = 0, .y = 0},
			.extent = extent,
		},
		.clearValueCount = uint32_t(clearValues.size()),
		.pClearValues = clearValues.data(),
	};

	vkCmdBeginRenderPass(commandBuffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

	VkRect2D scissor{
		.offset = {.x = 0, .y = 0},
		.extent = extent,
	};
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	VkViewport viewport{
		.x = 0.0f,
		.y = 0.0f,
		.width = float(extent.width),
		.height = float(extent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
}

void Renderpass::End(const CommandBuffer& commandBuffer)
{
	vkCmdEndRenderPass(commandBuffer);
}