#include "ShadowPipeline.hpp"
#include "backend/VulkanContext.hpp"
#include "core/Time.hpp"
#include "renderer/scene/SceneManager.hpp"
#include "renderer/components/CameraComponent.hpp"

ShadowPipeline::ShadowPipeline()
{
}

ShadowPipeline::~ShadowPipeline()
{
	if (offscreenPass.frameBuffer != VK_NULL_HANDLE) {
		vkDestroyFramebuffer(VulkanContext::GetDevice(), offscreenPass.frameBuffer, nullptr);
		offscreenPass.frameBuffer = VK_NULL_HANDLE;
	}

	if (offscreenPass.renderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(VulkanContext::GetDevice(), offscreenPass.renderPass, nullptr);
		offscreenPass.renderPass = VK_NULL_HANDLE;
	}

	m_BufferScene.Destroy();
	m_BufferOffscreen.Destroy();
}

void ShadowPipeline::CreateRenderPass()
{
	VkAttachmentDescription attachmentDescription{};
	attachmentDescription.format = offscreenDepthFormat;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear depth at beginning of the render pass
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;						// We will read from depth, so it's important to store the depth attachment results
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;					// We don't care about initial layout of the attachment
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;// Attachment will be transitioned to shader read at render pass end

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 0;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;			// Attachment will be used as depth/stencil during render pass

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 0;													// No color attachments
	subpass.pDepthStencilAttachment = &depthReference;									// Reference to our depth attachment

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo create_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &attachmentDescription,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = uint32_t(dependencies.size()),
		.pDependencies = dependencies.data(),
	};

	VulkanContext::VK_CHECK(
		vkCreateRenderPass(VulkanContext::GetDevice(), &create_info, nullptr, &offscreenPass.renderPass),
		"[Vulkan] Create Render pass failed"
	);
}

void ShadowPipeline::Rebuild()
{
}

void ShadowPipeline::CreatePipeline()
{
	PrepareOffscreenFrameBuffer();
	PrepareUniformBuffers();
	CreateDescriptors();
}

void ShadowPipeline::Prepare(const Scene* scene, const CommandBuffer& commandBuffer)
{
}

void ShadowPipeline::Render(const Scene* scene, const CommandBuffer& commandBuffer)
{
}

void ShadowPipeline::PrepareOffscreenFrameBuffer()
{
	offscreenPass.width = shadowMapize;
	offscreenPass.height = shadowMapize;

	m_DepthAttachment = std::make_unique<ImageDepth>(glm::uvec2{ shadowMapize, shadowMapize }, offscreenDepthFormat);

	CreateRenderPass();

	VkImageView view = m_DepthAttachment->getView();

	VkFramebufferCreateInfo create_info
	{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = offscreenPass.renderPass,
		.attachmentCount = 1,
		.pAttachments = &view,
		.width = uint32_t(offscreenPass.width),
		.height = uint32_t(offscreenPass.height),
		.layers = 1,
	};

	VulkanContext::VK_CHECK(
		vkCreateFramebuffer(VulkanContext::GetDevice(), &create_info, nullptr, &offscreenPass.frameBuffer),
		"[vulkan] Creating frame buffer failed"
	);

}

void ShadowPipeline::PrepareUniformBuffers()
{
	m_BufferScene = Buffer(
		sizeof(UniformDataScene),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		Buffer::Mapped
	);

	m_BufferOffscreen = Buffer(
		sizeof(UniformDataOffscreen),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		Buffer::Mapped
	);

	UpdateLight();
	UpdateUniforms();
}

void ShadowPipeline::UpdateUniforms()
{
	Camera* cam = SceneManager::Get()->getScene()->GetRenderCam()->camera();
	// update scene
	uniformDataScene.projection = cam->getProjectionMatrix();
	uniformDataScene.view = cam->getViewMatrix();
	uniformDataScene.model = glm::mat4(1.0f);
	uniformDataScene.lightPos = glm::vec4(lightPos, 1.0f);
	uniformDataScene.depthBiasMVP = uniformDataOffscreen.depthMVP;
	uniformDataScene.zNear = zNear;
	uniformDataScene.zFar = zFar;
	memcpy(m_BufferScene.data(), &uniformDataScene, sizeof(uniformDataScene));

	// update offscreen
	glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(lightFOV), 1.0f, zNear, zFar);
	glm::mat4 depthViewMatrix = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
	glm::mat4 depthModelMatrix = glm::mat4(1.0f);

	uniformDataOffscreen.depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

	memcpy(m_BufferOffscreen.data(), &uniformDataOffscreen, sizeof(uniformDataOffscreen));
}

void ShadowPipeline::CreateDescriptors()
{
}

void ShadowPipeline::UpdateLight()
{
	// Animate the light source
	lightPos.x = cos(glm::radians(Time::Now * 360.0f)) * 40.0f;
	lightPos.y = -50.0f + sin(glm::radians(Time::Now * 360.0f)) * 20.0f;
	lightPos.z = 25.0f + sin(glm::radians(Time::Now * 360.0f)) * 5.0f;
}
