#pragma once

#include <string>
#include "backend/VulkanContext.hpp"
#include "backend/images/ImageCube.hpp"

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

	void RunCPUBlit();

	void CreateComputePipeline();

	void Prepare();

	void ExecuteComputeShader();

	void SaveAsImage();

private:
	IBLUtilsApplicationSpecification specs;

	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_Pipeline;

	std::shared_ptr<ImageCube> inputImg;
	std::shared_ptr<ImageCube> storageImg;

	DescriptorAllocator m_DescriptorAllocator;

	VkDescriptorSetLayout set1_TextureLayout = VK_NULL_HANDLE;
	VkDescriptorSet set1_Texture = VK_NULL_HANDLE;

	VkFence fence;

	std::unique_ptr<Application> app;
};

