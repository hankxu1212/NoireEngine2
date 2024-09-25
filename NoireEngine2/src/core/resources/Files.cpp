#include "Files.hpp"

#include <fstream>
#include <iostream>
#include <filesystem>

#if defined(_WIN32)
#include <windows.h>
#include <Knownfolders.h>
#include <Shlobj.h>
#include <direct.h>
#include <io.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#include <sys/stat.h>
#endif //WINDOWS

std::vector<std::byte> Files::Read(const std::string& path)
{
	std::string name = Path(path);
	auto length = std::filesystem::file_size(name);
	if (length == 0) {
		std::cerr << "Found empty file: " << name << std::endl;
		return {};  // empty vector
	}

	std::vector<std::byte> buffer(length);
	std::ifstream inputFile(name, std::ios_base::binary);
	inputFile.read(reinterpret_cast<char*>(buffer.data()), length);
	inputFile.close();
	return buffer;
}

static std::string GetExecutableFile() 
{
#if defined(_WIN32)
	//See: https://stackoverflow.com/questions/1023306/finding-current-executables-path-without-proc-self-exe
	TCHAR buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::string ret = buffer;
	ret = ret.substr(0, ret.rfind('\\'));
	return ret;
#elif defined(__linux__)
	//From: https://stackoverflow.com/questions/933850/how-do-i-find-the-location-of-the-executable-in-c
	std::vector< char > buffer(1000);
	while (1) {
		ssize_t got = readlink("/proc/self/exe", &buffer[0], buffer.size());
		if (got <= 0) {
			return "";
		}
		else if (got < (ssize_t)buffer.size()) {
			std::string ret = std::string(buffer.begin(), buffer.begin() + got);
			return ret.substr(0, ret.rfind('/'));
		}
		buffer.resize(buffer.size() + 4000);
	}
#elif defined(__APPLE__)
	//From: https://stackoverflow.com/questions/799679/programmatically-retrieving-the-absolute-path-of-an-os-x-command-line-app/1024933
	uint32_t bufsize = 0;
	std::vector< char > buffer;
	_NSGetExecutablePath(&buffer[0], &bufsize);
	buffer.resize(bufsize, '\0');
	bufsize = buffer.size();
	if (_NSGetExecutablePath(&buffer[0], &bufsize) != 0) {
		throw std::runtime_error("Call to _NSGetExecutablePath failed for mysterious reasons.");
	}
	std::string ret = std::string(&buffer[0]);
	return ret.substr(0, ret.rfind('/'));
#else
#error "No idea what the OS is."
#endif
}

std::string Files::Path(const std::string& suffix)
{
	static std::string path = GetExecutableFile(); //cache result of GetExecutableFile()
	return path + "/" + suffix;
}

std::string Files::Path(const char* suffix)
{
	const std::string& strPath = suffix;
	return Path(strPath);
}

bool Files::Exists(const std::string& path)
{
	return std::filesystem::exists(path);
}
