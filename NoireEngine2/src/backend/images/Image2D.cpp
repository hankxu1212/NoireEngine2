#include "Image2D.hpp"

#include "core/resources/Resources.hpp"
#include "core/Bitmap.hpp"
#include "backend/buffers/Buffer.hpp"

std::shared_ptr<Image2D> Image2D::Create(const Node& node) {
	if (auto resource = Resources::Get().Find<Image2D>(node))
		return resource;

	auto result = std::make_shared<Image2D>("");
	Resources::Get().Add(node, std::dynamic_pointer_cast<Resource>(result));
	node >> *result;
	result->Load();
	return result;
}

std::shared_ptr<Image2D> Image2D::Create(const std::filesystem::path& filename, VkFilter filter, VkSamplerAddressMode addressMode, bool anisotropic, bool mipmap) {
	Image2D temp(filename, filter, addressMode, anisotropic, mipmap, false);
	Node node;
	node << temp;
	return Create(node);
}

Image2D::Image2D(std::filesystem::path filename, VkFilter filter, VkSamplerAddressMode addressMode, bool anisotropic, bool mipmap, bool load) :
	Image(filter, addressMode, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_FORMAT_R8G8B8A8_UNORM, 1, 1, { 0, 0, 1 }),
	filename(std::move(filename)),
	anisotropic(anisotropic),
	mipmap(mipmap) 
{
	if (load) {
		Image2D::Load();
	}
}

Image2D::Image2D(const glm::vec2 extent, VkFormat format, VkImageLayout layout, VkImageUsageFlags usage, VkFilter filter, VkSamplerAddressMode addressMode,
	VkSampleCountFlagBits samples, bool anisotropic, bool mipmap) :
	Image(filter, addressMode, samples, layout,
		usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		format, 1, 1, { static_cast<uint32_t>(extent.x), static_cast<uint32_t>(extent.y), 1 }),
	anisotropic(anisotropic),
	mipmap(mipmap),
	components(4) 
{
	Image2D::Load();
}

Image2D::Image2D(std::unique_ptr<Bitmap>&& bitmap, VkFormat format, VkImageLayout layout, VkImageUsageFlags usage, VkFilter filter, VkSamplerAddressMode addressMode,
	VkSampleCountFlagBits samples, bool anisotropic, bool mipmap) :
	Image(filter, addressMode, samples, layout,
		usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		format, 1, 1, { static_cast<uint32_t>(bitmap->size.x), static_cast<uint32_t>(bitmap->size.y), 1 }),
	anisotropic(anisotropic),
	mipmap(mipmap),
	components(bitmap->bytesPerPixel) 
{
	Image2D::Load(std::move(bitmap));
}

void Image2D::SetPixels(const uint8_t* pixels, uint32_t layerCount, uint32_t baseArrayLayer) {
	Buffer bufferStaging(extent.width * extent.height * components * arrayLayers, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* data;
	bufferStaging.MapMemory(&data);
	std::memcpy(data, pixels, bufferStaging.getSize());
	bufferStaging.UnmapMemory();

	CopyBufferToImage(bufferStaging.getBuffer(), image, extent, layerCount, baseArrayLayer);

	bufferStaging.Destroy();
}

void Image2D::Load(std::unique_ptr<Bitmap> loadBitmap) {
	if (!filename.empty() && !loadBitmap) {
		loadBitmap = std::make_unique<Bitmap>(filename);
		extent = { static_cast<uint32_t>(loadBitmap->size.x), static_cast<uint32_t>(loadBitmap->size.y), 1 };
		components = loadBitmap->bytesPerPixel;
	}

	if (extent.width == 0 || extent.height == 0)
		return;

	mipLevels = mipmap ? getMipLevels(extent) : 1;

	CreateImage(image, memory, extent, format, samples, VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mipLevels, arrayLayers, VK_IMAGE_TYPE_2D);
	CreateImageSampler(sampler, filter, addressMode, anisotropic, mipLevels);
	CreateImageView(image, view, VK_IMAGE_VIEW_TYPE_2D, format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayLayers, 0);

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

const Node& operator>>(const Node& node, Image2D& image) {
	node["filename"].Get(image.filename);
	node["filter"].Get(image.filter);
	node["addressMode"].Get(image.addressMode);
	node["anisotropic"].Get(image.anisotropic);
	node["mipmap"].Get(image.mipmap);
	return node;
}

Node& operator<<(Node& node, const Image2D& image) {
	node["filename"].Set(image.filename);
	node["filter"].Set(image.filter);
	node["addressMode"].Set(image.addressMode);
	node["anisotropic"].Set(image.anisotropic);
	node["mipmap"].Set(image.mipmap);
	return node;
}
