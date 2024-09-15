#include "Files.hpp"

#include <fstream>

std::optional<std::string> Files::Read(const std::filesystem::path& path)
{
	std::string pathStr = path.string();
	std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

	std::ifstream stream(path, std::ios::binary | std::ios::ate);

	if (!stream)
		return std::nullopt;

	std::streampos end = stream.tellg();
	stream.seekg(0, std::ios::beg);
	uint64_t size = end - stream.tellg();

	// File is empty
	if (size == 0)
	{
		return std::optional<std::string>();
	}

	char* buffer = new char[size];
	stream.read(buffer, size);
	stream.close();
	return buffer;
}