#pragma once

#include <string>
#include "backend/VulkanContext.hpp"
#include "backend/images/ImageCube.hpp"

struct IBLUtilsApplicationSpecification
{
	std::string inFile, outFile;
	bool isGGX = false;
	bool isHDR = true;
	uint32_t outdim = 512;
};

class IBLUtilsApplication
{
public:
	IBLUtilsApplication(IBLUtilsApplicationSpecification& specs_);

	void Run();

private:
	IBLUtilsApplicationSpecification specs;

	std::unique_ptr<Application> app;
};

