#include "Renderer.hpp"

#include "backend/VulkanContext.hpp"
#include <memory>

#include "renderer/scene/Scene.hpp"
static std::unique_ptr<Scene> scene = std::make_unique<Scene>();

Renderer::Renderer()
{
	glm::quat q{ 0,0,0,0 };
	glm::vec3 s{ 1,1,1 };

	glm::vec3 e1T{ 0,0,0 };
	scene->Instantiate(e1T, q, s);

	glm::vec3 e2T{ 5,0,0 };
	scene->Instantiate(e2T, q, s);

	glm::vec3 e3T{ 5,0,5 };
	scene->Instantiate(e3T, q, s);

	glm::vec3 e4T{ 0,0,5 };
	scene->Instantiate(e4T, q, s);

	glm::vec3 e5T{ 0,2,0 };
	scene->Instantiate(e5T, q, s);
	//e2->AddChild();
}

void Renderer::CreatePipelines()
{
	objectPipeline = std::make_unique<ObjectPipeline>(this);
}

void Renderer::CreateRenderPass()
{
	std::array< VkAttachmentDescription, 2 > attachments{
		VkAttachmentDescription { //0 - color attachment:
			.format = VulkanContext::Get().getSurface(0)->getFormat().format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		},
		VkAttachmentDescription{ //1 - depth attachment:
			.format = VK_FORMAT_D32_SFLOAT_S8_UINT,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		},
	};

	VkAttachmentReference color_attachment_ref{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentReference depth_attachment_ref{
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpass{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment_ref,
		.pDepthStencilAttachment = &depth_attachment_ref,
	};

	//this defers the image load actions for the attachments:
	std::array< VkSubpassDependency, 2 > dependencies{
		VkSubpassDependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		},
		VkSubpassDependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		}
	};

	VkRenderPassCreateInfo create_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = uint32_t(attachments.size()),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = uint32_t(dependencies.size()),
		.pDependencies = dependencies.data(),
	};

	VulkanContext::VK_CHECK(
		vkCreateRenderPass(VulkanContext::GetDevice(), &create_info, nullptr, &m_Renderpass),
		"[Vulkan] Create Render pass failed"
	);
}

void Renderer::Cleanup()
{
	objectPipeline.reset();
	
	DestroyFrameBuffers();

	if (m_Renderpass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(VulkanContext::GetDevice(), m_Renderpass, nullptr);
	}
}

void Renderer::Update()
{
	scene->Update();
	objectPipeline->Update(scene.get());
}

void Renderer::Render(const CommandBuffer& commandBuffer, uint32_t surfaceId)
{
	objectPipeline->Render(scene.get(), commandBuffer, surfaceId);
}

void Renderer::Rebuild()
{
	std::cout << "Rebuilt renderer and frame buffers\n";
	if (s_SwapchainDepthImage != nullptr && s_SwapchainDepthImage->getImage() != VK_NULL_HANDLE) {
		DestroyFrameBuffers();
	}

	// TODO: add support for multiple swapchains
	const SwapChain* swapchain = VulkanContext::Get().getSwapChain(0);
	s_SwapchainDepthImage = std::make_unique<ImageDepth>(swapchain->getExtentVec2(), VK_SAMPLE_COUNT_1_BIT);

	//Make framebuffers for each swapchain image:
	m_Framebuffers.assign(swapchain->getImageViews().size(), VK_NULL_HANDLE);
	for (size_t i = 0; i < swapchain->getImageViews().size(); ++i) {
		std::array< VkImageView, 2 > attachments{
			swapchain->getImageViews()[i],
			s_SwapchainDepthImage->getView()
		};
		VkFramebufferCreateInfo create_info{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = m_Renderpass,
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

void Renderer::DestroyFrameBuffers()
{
	for (VkFramebuffer& framebuffer : m_Framebuffers) 
	{
		assert(framebuffer != VK_NULL_HANDLE);
		vkDestroyFramebuffer(VulkanContext::GetDevice(), framebuffer, nullptr);
		framebuffer = VK_NULL_HANDLE;
	}
	m_Framebuffers.clear();
	s_SwapchainDepthImage.reset();
}
