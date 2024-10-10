#pragma once

#include <filesystem>

class Files
{
public:
	static std::vector<std::byte> Read(const std::string& pathFromExecutable);
	static std::string Path(const std::string& suffix, bool assert=true);
	static std::string Path(const char* suffix, bool assert=true);
	static bool Exists(const std::string& path);
};

