#pragma once

#include <filesystem>

#include "Image.hpp"
#include "core/resources/Resource.hpp"
#include "core/resources/nodes/Node.hpp"

class Image2D : public Image, public Resource 
{
public:
	// creates a texture from a file, loads it and caches it
	static std::shared_ptr<Image2D> Create(
		const std::filesystem::path& filename, 
		VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
		VkFilter filter = VK_FILTER_LINEAR,
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		bool anisotropic = true, bool mipmap = true, bool load=true
	);

	explicit Image2D();

	// loads an image with filename
	explicit Image2D(const std::filesystem::path& filename, VkFormat format, VkFilter filter, VkSamplerAddressMode addressMode, bool mipmap, bool load);

	// create an empty image with specified width, height, format, layout, and usage
	explicit Image2D(uint32_t w, uint32_t h, VkFormat format, VkImageLayout layout, VkImageUsageFlags usage, bool mipmap);

	void Load(std::unique_ptr<Bitmap> loadBitmap = nullptr, bool useSampler=true, bool useView=true);

	/**
	  * Sets the pixels of this image.
	  * @param pixels The pixels to copy from.
	  * @param layerCount The amount of layers contained in the pixels.
	  * @param baseArrayLayer The first layer to copy into.
	*/
	void SetPixels(const uint8_t* pixels, uint32_t layerCount, uint32_t baseArrayLayer);

	std::type_index getTypeIndex() const override { return typeid(Image2D); }

	const std::filesystem::path& GetFilename() const { return filename; }
	bool IsAnisotropic() const { return anisotropic; }
	bool IsMipmap() const { return mipmap; }
	uint32_t GetComponents() const { return components; }

	friend const Node& operator>>(const Node& node, Image2D& image);
	friend Node& operator<<(Node& node, const Image2D& image);

private:
	static std::shared_ptr<Image2D> Create(const Node& node);

	std::filesystem::path filename;

	bool anisotropic;
	bool mipmap;
	uint32_t components = 0;
};
