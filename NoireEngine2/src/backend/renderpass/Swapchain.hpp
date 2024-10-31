#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class PhysicalDevice;
class Surface;
class LogicalDevice;

class SwapChain 
{
public:
	SwapChain(
		const PhysicalDevice& physicalDevice, 
		Surface& surface, 
		const LogicalDevice& logicalDevice, 
		const VkExtent2D& extent, 
		const SwapChain* oldSwapchain = nullptr
	);
	~SwapChain();

	/**
		* Acquires the next image in the swapchain into the internal acquired image. The function will always Wait until the next image has been acquired by setting timeout to UINT64_MAX.
		* @param presentCompleteSemaphore A optional semaphore that is signaled when the image is ready for use.
		* @param fence A optional fence that is signaled once the previous command buffer has completed.
		* @return Result of the image acquisition.
		*/
	VkResult AcquireNextImage(const VkSemaphore& presentCompleteSemaphore = VK_NULL_HANDLE, VkFence fence = VK_NULL_HANDLE);

	/**
		* Queue an image for presentation using the internal acquired image for queue presentation.
		* @param presentQueue Presentation queue for presenting the image.
		* @param waitSemaphore A optional semaphore that is waited on before the image is presented.
		* @return Result of the queue presentation.
		*/
	VkResult QueuePresent(const VkQueue& presentQueue, const VkSemaphore& waitSemaphore = VK_NULL_HANDLE);

	bool IsSameExtent(const VkExtent2D& extent2D) { return extent.width == extent2D.width && extent.height == extent2D.height; }

	operator const VkSwapchainKHR& () const { return swapchain; }

	const VkExtent2D&					getExtent() const { return extent; }
	glm::uvec2							getExtentVec2() const { return { extent.width, extent.height }; }
	uint32_t							getImageCount() const { return imageCount; }
	VkSurfaceTransformFlagsKHR			getPreTransform() const { return preTransform; }
	VkCompositeAlphaFlagBitsKHR			getCompositeAlpha() const { return compositeAlpha; }
	const std::vector<VkImage>&			getImages() const { return images; }
	const VkImage&						getActiveImage() const { return images[activeImageIndex]; }
	const std::vector<VkImageView>&		getImageViews() const { return imageViews; }
	const VkSwapchainKHR&				getSwapchain() const { return swapchain; }
	uint32_t							getActiveImageIndex() const { return activeImageIndex; }

private:
	const PhysicalDevice& physicalDevice;
	const Surface& surface;
	const LogicalDevice& logicalDevice;

	VkExtent2D extent;
	VkPresentModeKHR presentMode;

	uint32_t imageCount = 0;
	VkSurfaceTransformFlagsKHR preTransform;
	VkCompositeAlphaFlagBitsKHR compositeAlpha;
	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;

	uint32_t activeImageIndex;
};
