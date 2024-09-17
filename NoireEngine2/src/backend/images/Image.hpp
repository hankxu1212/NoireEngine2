#pragma once

#include "backend/descriptor/Descriptor.hpp"
#include "glm/glm.hpp"

class CommandBuffer;
class Bitmap;
	
// A representation of a Vulkan image, sampler, and view.
class Image : public Descriptor {
public:
	Image(VkFilter filter, VkSamplerAddressMode addressMode, VkSampleCountFlagBits samples, VkImageLayout layout, VkImageUsageFlags usage,
		VkFormat format, uint32_t mipLevels, uint32_t arrayLayers, const VkExtent3D& extent);

	~Image();

	void Destroy();

	WriteDescriptorSet getWriteDescriptor(uint32_t binding, VkDescriptorType descriptorType, const std::optional<OffsetSize>& offsetSize) const override;

	static VkDescriptorSetLayoutBinding GetDescriptorSetLayout(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stage, uint32_t count);

	/**
	  * Copies the images pixels from memory to a bitmap. If this method is called from multiple threads at the same time Vulkan will crash!
	  * @param mipLevel The mipmap level index to sample.
	  * @param arrayLayer The array level to sample.
	  * @return A copy of the images pixels.
	*/
	std::unique_ptr<Bitmap>	getBitmap(uint32_t mipLevel = 0, uint32_t arrayLayer = 0) const;

	const VkExtent3D&		getExtent() const { return extent; }
	glm::vec2				getSize() const { return { extent.width, extent.height }; }
	VkFormat				getFormat() const { return format; }
	VkSampleCountFlagBits	getSamples() const { return samples; }
	VkImageUsageFlags		getUsage() const { return usage; }
	uint32_t				getMipLevels() const { return mipLevels; }
	uint32_t				getArrayLevels() const { return arrayLayers; }
	VkFilter				getFilter() const { return filter; }
	VkSamplerAddressMode	getAddressMode() const { return addressMode; }
	VkImageLayout			getLayout() const { return layout; }
	const VkImage&			getImage() { return image; }
	const VkDeviceMemory&	getMemory() { return memory; }
	const VkSampler&		getSampler() const { return sampler; }
	const VkImageView&		getView() const { return view; }

	static uint32_t			getMipLevels(const VkExtent3D& extent);

	static bool HasDepth(VkFormat format);

	static bool HasStencil(VkFormat format);

	static void CreateImage(VkImage& image, VkDeviceMemory& memory, const VkExtent3D& extent, VkFormat format, VkSampleCountFlagBits samples,
		VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, uint32_t mipLevels, uint32_t arrayLayers, VkImageType type);

	static void CreateImageSampler(VkSampler& sampler, VkFilter filter, VkSamplerAddressMode addressMode, bool anisotropic, uint32_t mipLevels);

	static void CreateImageView(const VkImage& image, VkImageView& imageView, VkImageViewType type, VkFormat format, VkImageAspectFlags imageAspect,
		uint32_t mipLevels, uint32_t baseMipLevel, uint32_t layerCount, uint32_t baseArrayLayer);

	static void CreateMipmaps(const VkImage& image, const VkExtent3D& extent, VkFormat format, VkImageLayout dstImageLayout, uint32_t mipLevels,
		uint32_t baseArrayLayer, uint32_t layerCount);

	static void TransitionImageLayout(const VkImage& image, VkFormat format, VkImageLayout srcImageLayout, VkImageLayout dstImageLayout,
		VkImageAspectFlags imageAspect, uint32_t mipLevels, uint32_t baseMipLevel, uint32_t layerCount, uint32_t baseArrayLayer);

	static void InsertImageMemoryBarrier(const CommandBuffer& commandBuffer, const VkImage& image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
		VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
		VkImageAspectFlags imageAspect, uint32_t mipLevels, uint32_t baseMipLevel, uint32_t layerCount, uint32_t baseArrayLayer);

	static void CopyBufferToImage(const VkBuffer& buffer, const VkImage& image, const VkExtent3D& extent, uint32_t layerCount, uint32_t baseArrayLayer);

	static bool CopyImage(const VkImage& srcImage, VkImage& dstImage, VkDeviceMemory& dstImageMemory, VkFormat srcFormat, const VkExtent3D& extent,
		VkImageLayout srcImageLayout, uint32_t mipLevel, uint32_t arrayLayer);

protected:
	VkExtent3D extent;
	VkSampleCountFlagBits samples;
	VkImageUsageFlags usage;
	VkFormat format = VK_FORMAT_UNDEFINED;
	uint32_t mipLevels = 0;
	uint32_t arrayLayers;

	VkFilter filter;
	VkSamplerAddressMode addressMode;

	VkImageLayout layout;

	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkSampler sampler = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
};
