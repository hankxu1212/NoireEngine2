#pragma once

#include <filesystem>

#include "Image.hpp"
#include "core/resources/Resource.hpp"
#include "core/resources/nodes/Node.hpp"

class Bitmap;

class ImageCube : public Image, public Resource {
public:
	ImageCube() = default;

	static std::shared_ptr<ImageCube> Create(const std::filesystem::path& filename, bool usingHDR = true, VkFilter filter = VK_FILTER_LINEAR,
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, bool anisotropic = true, bool mipmap = true);

	explicit ImageCube(std::filesystem::path filename, VkFilter filter=VK_FILTER_LINEAR, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 
		bool anisotropic=true, bool mipmap=true, bool usingHDR=true);

	explicit ImageCube(const std::filesystem::path& filename, VkFormat format, VkImageLayout layout, VkImageUsageFlags usage, bool usingHDR);

	explicit ImageCube(const glm::vec2 extent, VkFormat format, VkImageLayout layout, VkImageUsageFlags usage, bool usingHDR);
	
	/**
	  * Sets the pixels of this image.
	  * @param pixels The pixels to copy from.
	  * @param layerCount The amount of layers contained in the pixels.
	  * @param baseArrayLayer The first layer to copy into.
	*/
	void SetPixels(const uint8_t* pixels, uint32_t layerCount, uint32_t baseArrayLayer, uint32_t miplevel=0);

	void SaveAsPNG(const std::string& out, const glm::uvec2 size);

	std::type_index getTypeIndex() const override { return typeid(ImageCube); }

	std::filesystem::path filename;
	bool anisotropic;
	bool mipmap;
	uint32_t components = 0;
	bool isHDR = true;

private:
	static std::shared_ptr<ImageCube> Create(const Node& node);

	friend const Node& operator>>(const Node& node, ImageCube& image);
	friend Node& operator<<(Node& node, const ImageCube& image);

	void Load(std::unique_ptr<Bitmap> loadBitmap = nullptr);

};
