#pragma once

#include <string>

struct IBLUtilsApplicationSpecification
{
	std::string inFile, outFile;
	bool isGGX = false;
};

class IBLUtilsApplication
{
public:
	IBLUtilsApplication(IBLUtilsApplicationSpecification& specs_);
	~IBLUtilsApplication();

	void Run();

private:
	IBLUtilsApplicationSpecification specs;
};

