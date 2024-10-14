#include "ImageCube.hpp"

#include <cstring>

#include "core/resources/Resources.hpp"
#include "core/Bitmap.hpp"
#include "backend/buffers/Buffer.hpp"
#include "renderer/materials/MaterialLibrary.hpp"
#include "utils/Logger.hpp"
#include "core/resources/Files.hpp"

std::shared_ptr<ImageCube> ImageCube::Create(const Node& node) {
	if (auto resource = Resources::Get()->Find<ImageCube>(node))
		return resource;

	auto result = std::make_shared<ImageCube>("");
	Resources::Get()->Add(node, std::dynamic_pointer_cast<Resource>(result));
	node >> *result;
	result->Load();
	return result;
}

std::shared_ptr<ImageCube> ImageCube::Create(const std::filesystem::path& filename, bool usingHDR, VkFilter filter, VkSamplerAddressMode addressMode, bool anisotropic, bool mipmap)
{
	ImageCube temp(filename, filter, addressMode, anisotropic, mipmap, usingHDR);
	Node node;
	node << temp;
	return Create(node);
}

// by default, this loads a big HDR texture with VK_FORMAT_R32G32B32A32_SFLOAT
ImageCube::ImageCube(std::filesystem::path filename, VkFilter filter, VkSamplerAddressMode addressMode, 
	bool anisotropic, bool mipmap, bool usingHDR) :
	Image(filter, addressMode, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		usingHDR ? VK_FORMAT_R32G32B32A32_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM, 1, 6, { 0, 0, 1 }),
	filename(std::move(filename)),
	anisotropic(anisotropic),
	mipmap(mipmap),
	isHDR(usingHDR) {
}

ImageCube::ImageCube(const std::filesystem::path& filename, VkFormat format, VkImageLayout layout, VkImageUsageFlags usage, bool usingHDR) :
	filename(filename), 
	anisotropic(true),
	mipmap(false),
	isHDR(usingHDR),
	Image(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLE_COUNT_1_BIT, layout,
		usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		format, 1, 6, { 0, 0, 1 }) {
	Load();
}

ImageCube::ImageCube(const glm::vec2 extent, VkFormat format, VkImageLayout layout, VkImageUsageFlags usage, bool usingHDR) :
	anisotropic(true),
	mipmap(false),
	isHDR(usingHDR),
	Image(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLE_COUNT_1_BIT, layout,
		usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		format, 1, 6, { static_cast<uint32_t>(extent.x), static_cast<uint32_t>(extent.y), 1 })
{
	Load();
}

void ImageCube::SetPixels(const uint8_t* pixels, uint32_t layerCount, uint32_t baseArrayLayer, uint32_t miplevel) 
{
	VkExtent3D copyExtent = { extent.width >> miplevel, extent.height >> miplevel, extent.depth };
	Buffer bufferStaging(copyExtent.width * copyExtent.height * components * arrayLayers, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* data;
	bufferStaging.MapMemory(&data);
	memcpy(data, pixels, bufferStaging.getSize());
	bufferStaging.UnmapMemory();

	CopyBufferToImage(bufferStaging.getBuffer(), image, copyExtent, layerCount, baseArrayLayer, miplevel);

	bufferStaging.Destroy();
}

void ImageCube::SaveAsPNG(const std::string& out, const glm::uvec2 size)
{
	std::vector<uint8_t> allBytes;
	for (int i = 0; i < 6; i++)
	{
		auto bitmap = getBitmap(0, i, 4);

		size_t currOffset = allBytes.size();
		allBytes.resize(allBytes.size() + bitmap->GetLength());
		memcpy(allBytes.data() + currOffset, bitmap->data.get(), bitmap->GetLength());
	}
	if (allBytes.size() == 0) {
		NE_WARN("Written 0 bytes");
		return;
	}
	Bitmap::Write(Files::Path(out, false), allBytes.data(), size, 4);
}

const Node& operator>>(const Node& node, ImageCube& image) {
	node["filename"].Get(image.filename);
	node["filter"].Get(image.filter);
	node["addressMode"].Get(image.addressMode);
	node["anisotropic"].Get(image.anisotropic);
	node["mipmap"].Get(image.mipmap);
	node["mode"].Get(image.isHDR);
	node["format"].Get(image.format);
	return node;
}

Node& operator<<(Node& node, const ImageCube& image) {
	node["filename"].Set(image.filename);
	node["filter"].Set(image.filter);
	node["addressMode"].Set(image.addressMode);
	node["anisotropic"].Set(image.anisotropic);
	node["mipmap"].Set(image.mipmap);
	node["mode"].Set(image.isHDR);
	node["format"].Set(image.format);
	return node;
}

void ImageCube::Load(std::unique_ptr<Bitmap> loadBitmap) {
	if (!filename.empty() && !loadBitmap) {
		loadBitmap = std::make_unique<Bitmap>(filename, isHDR);
	}

	if (loadBitmap) {
		extent = { loadBitmap->size.x, loadBitmap->size.x, 1 };
		components = loadBitmap->bytesPerPixel;
	}

	if (extent.width == 0 || extent.height == 0) {
		return;
	}

	mipLevels = mipmap ? getMipLevels(extent) : 1;

	CreateImage(image, memory, extent, format, samples, VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mipLevels, arrayLayers, VK_IMAGE_TYPE_2D);
	CreateImageSampler(sampler, filter, addressMode, anisotropic, mipLevels);
	CreateImageView(image, view, VK_IMAGE_VIEW_TYPE_CUBE, format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayLayers, 0);

	if (loadBitmap || mipmap) {
		TransitionImageLayout(image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayLayers, 0);
	}

	if (loadBitmap) {
		Buffer bufferStaging(loadBitmap->GetLength(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		uint8_t* data;
		bufferStaging.MapMemory(reinterpret_cast<void**>(&data));
		std::memcpy(data, loadBitmap->data.get(), bufferStaging.getSize());
		bufferStaging.UnmapMemory();

		CopyBufferToImage(bufferStaging.getBuffer(), image, extent, arrayLayers, 0);

		bufferStaging.Destroy();
	}

	if (mipmap) {
		CreateMipmaps(image, extent, format, layout, mipLevels, 0, arrayLayers);
	}
	else if (loadBitmap) {
		TransitionImageLayout(image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayLayers, 0);
	}
	else {
		TransitionImageLayout(image, format, VK_IMAGE_LAYOUT_UNDEFINED, layout, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayLayers, 0);
	}
}
