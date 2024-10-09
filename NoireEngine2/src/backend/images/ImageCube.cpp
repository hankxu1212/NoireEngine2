#include "ImageCube.hpp"

#include <cstring>

#include "core/resources/Resources.hpp"
#include "core/Bitmap.hpp"
#include "backend/buffers/Buffer.hpp"
#include "renderer/materials/MaterialLibrary.hpp"
#include "utils/Logger.hpp"

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

void ImageCube::SetPixels(const uint8_t* pixels, uint32_t layerCount, uint32_t baseArrayLayer) 
{
	Buffer bufferStaging(extent.width * extent.height * components * arrayLayers, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* data;
	bufferStaging.MapMemory(&data);
	memcpy(data, pixels, bufferStaging.getSize());
	bufferStaging.UnmapMemory();

	CopyBufferToImage(bufferStaging.getBuffer(), image, extent, layerCount, baseArrayLayer);

	bufferStaging.Destroy();
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

inline glm::vec4 rgbe_to_float(glm::u8vec4 col) {
	//avoid decoding zero to a denormalized value:
	if (col == glm::u8vec4(0, 0, 0, 0)) return glm::vec4(0.0f);

	int exp = int(col.a) - 128;
	return glm::vec4(
		std::ldexp((col.r + 0.5f) / 256.0f, exp),
		std::ldexp((col.g + 0.5f) / 256.0f, exp),
		std::ldexp((col.b + 0.5f) / 256.0f, exp),
		1
	);
}

void ImageCube::Load(std::unique_ptr<Bitmap> loadBitmap) {
	if (!filename.empty() && !loadBitmap) {
		if (isHDR) {
			auto rawBitmap = Bitmap(filename); // in rgbe format

			// Reinterpret the data as an array of glm::u8vec4
			glm::u8vec4* vec4_data = reinterpret_cast<glm::u8vec4*>(rawBitmap.data.get());

			size_t num_vec4_elements = rawBitmap.GetLength() / sizeof(glm::u8vec4);  // Make sure to calculate the correct size

			std::vector<glm::vec4> rgbData;

			for (size_t i = 0; i < num_vec4_elements; ++i) {
				glm::u8vec4 color = vec4_data[i];
				rgbData.emplace_back(rgbe_to_float(color));
			}

			// make a new bitmap
			loadBitmap = std::make_unique<Bitmap>(std::make_unique<uint8_t[]>(rgbData.size() * sizeof(glm::vec4)), rawBitmap.size, 16);
			memcpy(loadBitmap->data.get(), rgbData.data(), rgbData.size() * sizeof(glm::vec4));
		}
		else { // load as png
			loadBitmap = std::make_unique<Bitmap>(filename);
		}
	}

	extent = { loadBitmap->size.x, loadBitmap->size.x, 1 };
	components = loadBitmap->bytesPerPixel;

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
