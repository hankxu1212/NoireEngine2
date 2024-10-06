#pragma once

#include <filesystem>

#include "Image.hpp"
#include "core/resources/Resource.hpp"
#include "core/resources/nodes/Node.hpp"

class Bitmap;

class ImageCube : public Image, public Resource {
public:
	static std::shared_ptr<ImageCube> Create(const Node& node);

	static std::shared_ptr<ImageCube> Create(const std::filesystem::path& filename, const std::string& fileSuffix, VkFilter filter = VK_FILTER_LINEAR,
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, bool anisotropic = true, bool mipmap = true);

	explicit ImageCube(std::filesystem::path filename, std::string fileSuffix = ".png", VkFilter filter = VK_FILTER_LINEAR,
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, bool anisotropic = true, bool mipmap = true, bool load = true);

	explicit ImageCube(const glm::vec2 extent, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		VkFilter filter = VK_FILTER_LINEAR, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT, bool anisotropic = false, bool mipmap = false);

	// load from bitmap
	explicit ImageCube(std::unique_ptr<Bitmap>&& bitmap, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		VkFilter filter = VK_FILTER_LINEAR, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT, bool anisotropic = false, bool mipmap = false);

	/**
	  * Sets the pixels of this image.
	  * @param pixels The pixels to copy from.
	  * @param layerCount The amount of layers contained in the pixels.
	  * @param baseArrayLayer The first layer to copy into.
	*/
	void SetPixels(const uint8_t* pixels, uint32_t layerCount, uint32_t baseArrayLayer);

	std::type_index getTypeIndex() const override { return typeid(ImageCube); }

	const std::filesystem::path& GetFilename() const { return filename; }
	const std::string& GetFileSuffix() const { return fileSuffix; }
	const std::vector<std::string>& GetFileSides() const { return fileSides; }
	bool IsAnisotropic() const { return anisotropic; }
	bool IsMipmap() const { return mipmap; }
	uint32_t GetComponents() const { return components; }

private:
	friend const Node& operator>>(const Node& node, ImageCube& image);
	friend Node& operator<<(Node& node, const ImageCube& image);

	void Load(std::unique_ptr<Bitmap> loadBitmap = nullptr);

	std::filesystem::path filename;
	std::string fileSuffix;

	// X, -X, +Y, -Y, +Z, -Z
	std::vector<std::string> fileSides = { "Right", "Left", "Top", "Bottom", "Back", "Front" };

	bool anisotropic;
	bool mipmap;
	uint32_t components = 0;
};
