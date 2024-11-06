#pragma once

#include <filesystem>
#include "glm/glm.hpp"
#include "core/resources/Files.hpp"

class Bitmap {
public:
	Bitmap() = default;
	Bitmap(const std::filesystem::path& filename, bool HDR=false);
	Bitmap(const glm::vec2 size, uint32_t bytesPerPixel = 4);
	Bitmap(std::unique_ptr<uint8_t[]>&& _data, const glm::vec2 _size, uint32_t _bytesPerPixel = 4);
	~Bitmap() = default;

	void Load(const std::filesystem::path& filename);
	void LoadHDR(const std::filesystem::path& filename);
	void Write(const std::filesystem::path& filename);

	static void Write(const std::filesystem::path& filename, const uint8_t* pixels, const glm::uvec2 size, uint32_t bytesPerPixel);

	uint32_t GetLength() const { return size.x * size.y * bytesPerPixel; }

	std::unique_ptr<uint8_t[]> data;
	glm::uvec2 size;
	uint32_t bytesPerPixel = 4;
};
