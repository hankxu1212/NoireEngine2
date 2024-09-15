#include "Bitmap.hpp"

#include <fstream>

#ifndef __STDC_LIB_EXT1__
#define __STDC_LIB_EXT1__
#endif

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif 

#include <utils/stb_image.h>

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif

#include <utils/stb_image_write.h> 

#include "core/resources/Files.hpp"

Bitmap::Bitmap(std::filesystem::path filename) {
	Load(filename);
}

Bitmap::Bitmap(const glm::vec2 size, uint32_t bytesPerPixel) :
	data(std::make_unique<uint8_t[]>(size_t(size.x * size.y* bytesPerPixel))),
	size(size),
	bytesPerPixel(bytesPerPixel) {
}

Bitmap::Bitmap(std::unique_ptr<uint8_t[]>&& _data, const glm::vec2 _size, uint32_t _bytesPerPixel) :
	data(std::move(_data)),
	size(_size),
	bytesPerPixel(_bytesPerPixel) {
}

void Bitmap::Load(const std::filesystem::path& filename) {
	auto fileLoaded = Files::Read(filename);

	if (!fileLoaded) {
		std::runtime_error(std::format("[vulkan]: Bitmap could not be loaded: {}", filename.string()));
		return;
	}

	data = std::unique_ptr<uint8_t[]>(stbi_load_from_memory(reinterpret_cast<uint8_t*>(fileLoaded->data()), static_cast<uint32_t>(fileLoaded->size()),
		reinterpret_cast<int32_t*>(&size.x), reinterpret_cast<int32_t*>(&size.y), reinterpret_cast<int32_t*>(&bytesPerPixel), STBI_rgb_alpha));
	if (!data)
		std::runtime_error(std::format("[vulkan]: stbi load from memory failed: {}", filename.string()));
	bytesPerPixel = 4;
}

void Bitmap::Write(const std::filesystem::path& filename) 
{
	if (auto parentPath = filename.parent_path(); !parentPath.empty())
		std::filesystem::create_directories(parentPath);

	std::ofstream os(filename, std::ios::binary | std::ios::out);
	int32_t len;
	std::unique_ptr<uint8_t[]> png(stbi_write_png_to_mem(data.get(), size.x * bytesPerPixel, size.x, size.y, bytesPerPixel, &len));
	os.write(reinterpret_cast<char*>(png.get()), len);
}