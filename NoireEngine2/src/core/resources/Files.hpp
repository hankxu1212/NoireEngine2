#pragma once

#include <filesystem>

class Files
{
public:
	static std::vector<std::byte> Read(const std::string& pathFromExecutable);
	static std::string Path(const std::string& suffix);
};

