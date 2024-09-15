#pragma once

#include <filesystem>
#include "glm/glm.hpp"

class Bitmap {
public:
	Bitmap() = default;
	Bitmap(std::filesystem::path filename);
	Bitmap(const glm::vec2 size, uint32_t bytesPerPixel = 4);
	Bitmap(std::unique_ptr<uint8_t[]>&& _data, const glm::vec2 _size, uint32_t _bytesPerPixel = 4);
	~Bitmap() = default;

	void Load(const std::filesystem::path& filename);
	void Write(const std::filesystem::path& filename);

	operator bool() const noexcept { return !data; }

	uint32_t GetLength() const { return size.x * size.y * bytesPerPixel; }

public:
	std::unique_ptr<uint8_t[]> data;
	glm::uvec2 size;
	uint32_t bytesPerPixel = 0;
};
