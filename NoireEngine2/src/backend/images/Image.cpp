#include "Image.hpp"

#include "core/Bitmap.hpp"
#include "backend/VulkanContext.hpp"
#include "core/resources/Files.hpp"

constexpr static float ANISOTROPY = 16.0f;

Image::Image(VkFilter filter, VkSamplerAddressMode addressMode, VkSampleCountFlagBits samples, VkImageLayout layout, VkImageUsageFlags usage, VkFormat format, uint32_t mipLevels,
	uint32_t arrayLayers, const VkExtent3D& extent) :
	extent(extent),
	samples(samples),
	usage(usage),
	format(format),
	mipLevels(mipLevels),
	arrayLayers(arrayLayers),
	filter(filter),
	addressMode(addressMode),
	layout(layout) {
}

Image::~Image() {
	Destroy();
}

void Image::Destroy()
{
	auto logicalDevice = VulkanContext::GetDevice();
	if (view != VK_NULL_HANDLE) {
		vkDestroyImageView(logicalDevice, view, nullptr);
		view = VK_NULL_HANDLE;
	}

	if (sampler != VK_NULL_HANDLE) {
		vkDestroySampler(logicalDevice, sampler, nullptr);
		sampler = VK_NULL_HANDLE;
	}
	
	if (memory != VK_NULL_HANDLE) {
		vkFreeMemory(logicalDevice, memory, nullptr);
		memory = VK_NULL_HANDLE;
	}

	if (image != VK_NULL_HANDLE) {
		vkDestroyImage(logicalDevice, image, nullptr);
		image = VK_NULL_HANDLE;
	}
}

VkDescriptorImageInfo Image::GetDescriptorInfo() const
{
	VkDescriptorImageInfo imageInfo {
		.sampler = sampler,
		.imageView = view,
		.imageLayout = layout
	};

	return imageInfo;
}

std::unique_ptr<Bitmap> Image::getBitmap(uint32_t mipLevel, uint32_t arrayLayer, uint32_t bytesPerPixel) const {
	auto logicalDevice = VulkanContext::GetDevice();

	glm::vec2 size(int32_t(extent.width >> mipLevel), int32_t(extent.height >> mipLevel));

	VkImage dstImage;
	VkDeviceMemory dstImageMemory;
	CopyImage(image, dstImage, dstImageMemory, format, { static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y),  1 }, layout, mipLevel, arrayLayer, 1);

	VkImageSubresource dstImageSubresource = {};
	dstImageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	dstImageSubresource.mipLevel = 0;
	dstImageSubresource.arrayLayer = 0;

	VkSubresourceLayout dstSubresourceLayout;
	vkGetImageSubresourceLayout(logicalDevice, dstImage, &dstImageSubresource, &dstSubresourceLayout);

	auto bitmap = std::make_unique<Bitmap>(std::make_unique<uint8_t[]>(dstSubresourceLayout.size), size, bytesPerPixel);

	void* data;
	vkMapMemory(logicalDevice, dstImageMemory, dstSubresourceLayout.offset, dstSubresourceLayout.size, 0, &data);
	std::memcpy(bitmap->data.get(), data, static_cast<std::size_t>(dstSubresourceLayout.size));
	vkUnmapMemory(logicalDevice, dstImageMemory);

	vkFreeMemory(logicalDevice, dstImageMemory, nullptr);
	vkDestroyImage(logicalDevice, dstImage, nullptr);

	return bitmap;
}

uint32_t Image::getMipLevels(const VkExtent3D& extent) {
	return static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, std::max(extent.height, extent.depth)))) + 1);
}

bool Image::HasDepth(VkFormat format) {
	static const std::vector<VkFormat> DEPTH_FORMATS = {
		VK_FORMAT_D16_UNORM, VK_FORMAT_X8_D24_UNORM_PACK32, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT
	};
	return std::find(DEPTH_FORMATS.begin(), DEPTH_FORMATS.end(), format) != std::end(DEPTH_FORMATS);
}

bool Image::HasStencil(VkFormat format) {
	static const std::vector<VkFormat> STENCIL_FORMATS = { VK_FORMAT_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT };
	return std::find(STENCIL_FORMATS.begin(), STENCIL_FORMATS.end(), format) != std::end(STENCIL_FORMATS);
}

void Image::CreateImage(VkImage& image, VkDeviceMemory& memory, const VkExtent3D& extent, VkFormat format, VkSampleCountFlagBits samples,
	VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, uint32_t mipLevels, uint32_t arrayLayers, VkImageType type) {

	auto logicalDevice = VulkanContext::GetDevice();

	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.flags = arrayLayers == 6 ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	imageCreateInfo.imageType = type;
	imageCreateInfo.format = format;
	imageCreateInfo.extent = extent;
	imageCreateInfo.mipLevels = mipLevels;
	imageCreateInfo.arrayLayers = arrayLayers;
	imageCreateInfo.samples = samples;
	imageCreateInfo.tiling = tiling;
	imageCreateInfo.usage = usage;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VulkanContext::VK(vkCreateImage(logicalDevice, &imageCreateInfo, nullptr, &image), "[vulkan] Error: failed to create image!");

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(logicalDevice, image, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = VulkanContext::FindMemoryType(memoryRequirements.memoryTypeBits, properties);
	VulkanContext::VK(vkAllocateMemory(logicalDevice, &memoryAllocateInfo, nullptr, &memory), "[vulkan] Error: cannot allocate image memory.");

	VulkanContext::VK(vkBindImageMemory(logicalDevice, image, memory, 0), "[vulkan] Error: cannot bind image memory.");
}

void Image::CreateImageSampler(VkSampler& sampler, VkFilter filter, VkSamplerAddressMode addressMode, bool anisotropic, uint32_t mipLevels) {
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = filter;
	samplerCreateInfo.minFilter = filter;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = addressMode;
	samplerCreateInfo.addressModeV = addressMode;
	samplerCreateInfo.addressModeW = addressMode;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.anisotropyEnable = anisotropic ? VK_TRUE : VK_FALSE;
	samplerCreateInfo.maxAnisotropy =
		(anisotropic && VulkanContext::Get()->getLogicalDevice()->getEnabledFeatures().samplerAnisotropy) ? min(ANISOTROPY, VulkanContext::Get()->getPhysicalDevice()->getProperties().limits.maxSamplerAnisotropy) : 1.0f;
	//samplerCreateInfo.compareEnable = VK_FALSE;
	//samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = static_cast<float>(mipLevels);
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	VulkanContext::VK(vkCreateSampler(VulkanContext::GetDevice(), &samplerCreateInfo, nullptr, &sampler),
		"[vulkan] Error: failed to create texture sampler.");
}

void Image::CreateImageView(const VkImage& image, VkImageView& imageView, VkImageViewType type, VkFormat format, VkImageAspectFlags imageAspect,
	uint32_t mipLevels, uint32_t baseMipLevel, uint32_t layerCount, uint32_t baseArrayLayer) {

	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.viewType = type;
	imageViewCreateInfo.format = format;
	imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	imageViewCreateInfo.subresourceRange.aspectMask = imageAspect;
	imageViewCreateInfo.subresourceRange.baseMipLevel = baseMipLevel;
	imageViewCreateInfo.subresourceRange.levelCount = mipLevels;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
	imageViewCreateInfo.subresourceRange.layerCount = layerCount;
	VulkanContext::VK(vkCreateImageView(VulkanContext::GetDevice(), &imageViewCreateInfo, nullptr, &imageView),
		"[vulkan] Cannot create image view");
}

void Image::CreateMipmaps(const VkImage& image, const VkExtent3D& extent, VkFormat format, VkImageLayout dstImageLayout, uint32_t mipLevels,
	uint32_t baseArrayLayer, uint32_t layerCount) {

	// Get device properites for the requested Image format.
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(*(VulkanContext::Get()->getPhysicalDevice()), format, &formatProperties);

	// Mip-chain generation requires support for blit source and destination
	assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);
	assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);

	CommandBuffer commandBuffer(true, VK_QUEUE_TRANSFER_BIT);

	for (uint32_t i = 1; i < mipLevels; i++) {
		VkImageMemoryBarrier barrier0 = {};
		barrier0.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier0.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier0.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier0.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier0.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier0.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier0.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier0.image = image;
		barrier0.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier0.subresourceRange.baseMipLevel = i - 1;
		barrier0.subresourceRange.levelCount = 1;
		barrier0.subresourceRange.baseArrayLayer = baseArrayLayer;
		barrier0.subresourceRange.layerCount = layerCount;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier0);

		VkImageBlit imageBlit = {};
		imageBlit.srcOffsets[1] = { int32_t(extent.width >> (i - 1)), int32_t(extent.height >> (i - 1)), 1 };
		imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.srcSubresource.mipLevel = i - 1;
		imageBlit.srcSubresource.baseArrayLayer = baseArrayLayer;
		imageBlit.srcSubresource.layerCount = layerCount;
		imageBlit.dstOffsets[1] = { int32_t(extent.width >> i), int32_t(extent.height >> i), 1 };
		imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.dstSubresource.mipLevel = i;
		imageBlit.dstSubresource.baseArrayLayer = baseArrayLayer;
		imageBlit.dstSubresource.layerCount = layerCount;
		vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

		VkImageMemoryBarrier barrier1 = {};
		barrier1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier1.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier1.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier1.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier1.newLayout = dstImageLayout;
		barrier1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier1.image = image;
		barrier1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier1.subresourceRange.baseMipLevel = i - 1;
		barrier1.subresourceRange.levelCount = 1;
		barrier1.subresourceRange.baseArrayLayer = baseArrayLayer;
		barrier1.subresourceRange.layerCount = layerCount;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier1);
	}

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = dstImageLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
	barrier.subresourceRange.layerCount = layerCount;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	commandBuffer.SubmitIdle();
}

void Image::TransitionImageLayout(const VkImage& image, VkFormat format, VkImageLayout srcImageLayout, VkImageLayout dstImageLayout,
	VkImageAspectFlags imageAspect, uint32_t mipLevels, uint32_t baseMipLevel, uint32_t layerCount, uint32_t baseArrayLayer) 
{
	// Create a command buffer for the transition
	CommandBuffer commandBuffer(true, VK_QUEUE_TRANSFER_BIT);

	// Set up access masks and pipeline stages based on source layout
	VkAccessFlags srcAccessMask = 0;
	VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	switch (srcImageLayout) {
	case VK_IMAGE_LAYOUT_UNDEFINED:
		srcAccessMask = 0;
		srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		break;
	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		srcStageMask = VK_PIPELINE_STAGE_HOST_BIT;
		break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		break;
	default:
		throw std::runtime_error("Unsupported image layout transition source");
	}

	// Set up access masks and pipeline stages based on destination layout
	VkAccessFlags dstAccessMask = 0;
	VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	switch (dstImageLayout) {
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		break;
	case VK_IMAGE_LAYOUT_GENERAL:
		dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		break;
	default:
		throw std::runtime_error("Unsupported image layout transition destination");
	}

	// Insert image memory barrier
	InsertImageMemoryBarrier(
		commandBuffer,
		image,
		srcAccessMask,
		dstAccessMask,
		srcImageLayout,
		dstImageLayout,
		srcStageMask,
		dstStageMask,
		imageAspect,
		mipLevels,
		baseMipLevel,
		layerCount,
		baseArrayLayer
	);

	// Submit the command buffer and wait for completion
	commandBuffer.SubmitIdle();
}

void Image::TransitionLayout(VkImageLayout srcLayout, VkImageLayout dstLayout, VkImageAspectFlags aspect)
{
	TransitionImageLayout(image, format, srcLayout, dstLayout, aspect, mipLevels, 0, arrayLayers, 0);
}

void Image::InsertImageMemoryBarrier(const CommandBuffer& commandBuffer, const VkImage& image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
	VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
	VkImageAspectFlags imageAspect, uint32_t mipLevels, uint32_t baseMipLevel, uint32_t layerCount, uint32_t baseArrayLayer) 
{
	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcAccessMask = srcAccessMask;
	imageMemoryBarrier.dstAccessMask = dstAccessMask;
	imageMemoryBarrier.oldLayout = oldImageLayout;
	imageMemoryBarrier.newLayout = newImageLayout;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange.aspectMask = imageAspect;
	imageMemoryBarrier.subresourceRange.baseMipLevel = baseMipLevel;
	imageMemoryBarrier.subresourceRange.levelCount = mipLevels;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = baseArrayLayer;
	imageMemoryBarrier.subresourceRange.layerCount = layerCount;
	vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, VK_DEPENDENCY_DEVICE_GROUP_BIT, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}

void Image::CopyBufferToImage(const VkBuffer& buffer, const VkImage& image, const VkExtent3D& extent, uint32_t layerCount, uint32_t baseArrayLayer, uint32_t miplevel) 
{
	CommandBuffer commandBuffer(true, VK_QUEUE_TRANSFER_BIT);

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = miplevel;
	region.imageSubresource.baseArrayLayer = baseArrayLayer;
	region.imageSubresource.layerCount = layerCount;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = extent;
	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	commandBuffer.SubmitIdle();
}

bool Image::CopyImage(const VkImage& srcImage, VkImage& dstImage, VkDeviceMemory& dstImageMemory, VkFormat srcFormat, const VkExtent3D& extent,
	VkImageLayout srcImageLayout, uint32_t mipLevel, uint32_t arrayLayer, uint32_t numLayers) {
	auto physicalDevice = VulkanContext::Get()->getPhysicalDevice();
	auto surface = VulkanContext::Get()->getSurface(0);

	bool supportsBlit = true;

	// no surface found, use copy
	if (!surface) {
		supportsBlit = false;
		goto start;
	}

	// Check if the device supports blitting from optimal images (the swapchain images are in optimal format).
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(*physicalDevice, surface->getFormat().format, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
		std::cout << "[vulkan] Device does not support blitting from optimal tiled images, using copy instead of blit!";
		supportsBlit = false;
	}

	// Check if the device supports blitting to linear images.
	vkGetPhysicalDeviceFormatProperties(*physicalDevice, srcFormat, &formatProperties);

	if (!(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
		std::cout << "Device does not support blitting to linear tiled images, using copy instead of blit!";
		supportsBlit = false;
	}

start:
	CreateImage(dstImage, dstImageMemory, extent, VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_LINEAR,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1, numLayers, VK_IMAGE_TYPE_2D);

	CommandBuffer commandBuffer(true, VK_QUEUE_TRANSFER_BIT);

	// Transition destination image to transfer destination layout.
	InsertImageMemoryBarrier(commandBuffer, dstImage, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_IMAGE_ASPECT_COLOR_BIT, 1, 0, 1, 0);

	// Transition image from previous usage to transfer source layout
	InsertImageMemoryBarrier(commandBuffer, srcImage, VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT, srcImageLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_IMAGE_ASPECT_COLOR_BIT, 1, mipLevel, 1, arrayLayer);

	// If source and destination support blit we'll blit as this also does automatic format conversion (e.g. from BGR to RGB).
	if (supportsBlit) {
		// Define the region to blit (we will blit the whole swapchain image).
		VkOffset3D blitSize = { static_cast<int32_t>(extent.width), static_cast<int32_t>(extent.height), static_cast<int32_t>(extent.depth) };

		VkImageBlit imageBlitRegion = {};
		imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.srcSubresource.mipLevel = mipLevel;
		imageBlitRegion.srcSubresource.baseArrayLayer = arrayLayer;
		imageBlitRegion.srcSubresource.layerCount = numLayers;
		imageBlitRegion.srcOffsets[1] = blitSize;
		imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.dstSubresource.mipLevel = 0;
		imageBlitRegion.dstSubresource.baseArrayLayer = 0;
		imageBlitRegion.dstSubresource.layerCount = numLayers;
		imageBlitRegion.dstOffsets[1] = blitSize;
		vkCmdBlitImage(commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlitRegion, VK_FILTER_NEAREST);
	}
	else {
		// Otherwise use image copy (requires us to manually flip components).
		VkImageCopy imageCopyRegion = {};
		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.mipLevel = mipLevel;
		imageCopyRegion.srcSubresource.baseArrayLayer = arrayLayer;
		imageCopyRegion.srcSubresource.layerCount = numLayers;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.mipLevel = 0;
		imageCopyRegion.dstSubresource.baseArrayLayer = 0;
		imageCopyRegion.dstSubresource.layerCount = numLayers;
		imageCopyRegion.extent = extent;
		vkCmdCopyImage(commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);
	}

	// Transition destination image to general layout, which is the required layout for mapping the image memory later on.
	InsertImageMemoryBarrier(commandBuffer, dstImage, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_IMAGE_ASPECT_COLOR_BIT, 1, 0, 1, 0);

	// Transition back the image after the blit is done.
	InsertImageMemoryBarrier(commandBuffer, srcImage, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcImageLayout,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_IMAGE_ASPECT_COLOR_BIT, 1, mipLevel, 1, arrayLayer);

	commandBuffer.SubmitIdle();

	return supportsBlit;
}
