#include <array>

#include "backend/VulkanContext.hpp"
#include "backend/images/Image.hpp"
#include "math/Math.hpp"

static const std::vector<VkCompositeAlphaFlagBitsKHR> COMPOSITE_ALPHA_FLAGS = {
	VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
	VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
};

SwapChain::SwapChain(const PhysicalDevice& physicalDevice, Surface& surface, const LogicalDevice& logicalDevice, const VkExtent2D& extent, const SwapChain* oldSwapchain) :
	physicalDevice(physicalDevice),
	surface(surface),
	logicalDevice(logicalDevice),
	extent(extent),
	presentMode(VK_PRESENT_MODE_FIFO_KHR),
	preTransform(VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR),
	compositeAlpha(VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR),
	activeImageIndex(UINT32_MAX) 
{
	auto& surfaceFormat = surface.getFormat();
	surface.UpdateCapabilities();
	auto& surfaceCapabilities = surface.getCapabilities();
	auto graphicsFamily = logicalDevice.getGraphicsFamily();
	auto presentFamily = logicalDevice.getPresentFamily();

	uint32_t physicalPresentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &physicalPresentModeCount, nullptr);
	std::vector<VkPresentModeKHR> physicalPresentModes(physicalPresentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &physicalPresentModeCount, physicalPresentModes.data());

	// choose present mode
	for (const auto& mode : physicalPresentModes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			this->presentMode = mode;
			break;
		}

		if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			this->presentMode = mode;
		}
	}

	// image counts
	auto desiredImageCount = surfaceCapabilities.minImageCount + 1;
	if (surfaceCapabilities.maxImageCount > 0 && desiredImageCount > surfaceCapabilities.maxImageCount) {
		desiredImageCount = surfaceCapabilities.maxImageCount;
	}

	// capabilities and extent
	if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
		// We prefer a non-rotated transform.
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else {
		preTransform = surfaceCapabilities.currentTransform;
	}

	for (const auto& compositeAlphaFlag : COMPOSITE_ALPHA_FLAGS) {
		if (surfaceCapabilities.supportedCompositeAlpha & compositeAlphaFlag) {
			compositeAlpha = compositeAlphaFlag;
			break;
		}
	}

	//VkSwapchainPresentScalingCreateInfoEXT swapchainScalingCreateInfo {
	//	.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_SCALING_CREATE_INFO_EXT,
	//	.scalingBehavior = VK_PRESENT_SCALING_ONE_TO_ONE_BIT_EXT
	//};

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	//swapchainCreateInfo.pNext = &swapchainScalingCreateInfo;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = desiredImageCount;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = this->extent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.preTransform = static_cast<VkSurfaceTransformFlagBitsKHR>(preTransform);
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.compositeAlpha = compositeAlpha;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
		swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	if (oldSwapchain)
		swapchainCreateInfo.oldSwapchain = oldSwapchain->swapchain;

	if (graphicsFamily != presentFamily) {
		std::array<uint32_t, 2> queueFamily = { graphicsFamily, presentFamily };
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamily.size());
		swapchainCreateInfo.pQueueFamilyIndices = queueFamily.data();
	}

	VulkanContext::VK_CHECK(vkCreateSwapchainKHR(logicalDevice, &swapchainCreateInfo, nullptr, &swapchain),
		"[vulkan] Error creating swapchain");

	VulkanContext::VK_CHECK(vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, nullptr),
		"[vulkan] Error get swapchain image count");
	images.resize(imageCount);
	imageViews.resize(imageCount);
	VulkanContext::VK_CHECK(vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, images.data()),
		"[vulkan] Error creating swapchain images");

	for (uint32_t i = 0; i < imageCount; i++) {
		Image::CreateImageView(images.at(i), imageViews.at(i), VK_IMAGE_VIEW_TYPE_2D, surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT,
			1, 0, 1, 0);
	}

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr, &fenceImage);
}

SwapChain::~SwapChain() {
	for (const auto& imageView : imageViews) {
		vkDestroyImageView(logicalDevice, imageView, nullptr);
	}
	vkDestroyFence(logicalDevice, fenceImage, nullptr);
	vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
}

VkResult SwapChain::AcquireNextImage(const VkSemaphore& presentCompleteSemaphore, VkFence fence) {
	if (fence != VK_NULL_HANDLE)
		VulkanContext::VK_CHECK(vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, UINT64_MAX),
			"[vulkan] Error waiting for fences while trying to acquire next image in swapchain");

	return vkAcquireNextImageKHR(logicalDevice, swapchain, UINT64_MAX, presentCompleteSemaphore, VK_NULL_HANDLE, &activeImageIndex);
}

VkResult SwapChain::QueuePresent(const VkQueue& presentQueue, const VkSemaphore& waitSemaphore) {
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &waitSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &activeImageIndex;
	return vkQueuePresentKHR(presentQueue, &presentInfo);
}
