#pragma once

#include <filesystem>

class Files
{
public:
	static std::optional<std::string> Read(const std::filesystem::path& path);
};

