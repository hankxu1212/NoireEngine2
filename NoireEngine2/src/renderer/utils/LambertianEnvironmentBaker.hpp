#pragma once

#include "backend/VulkanContext.hpp"
#include "backend/images/ImageCube.hpp"

struct IBLUtilsApplicationSpecification;

class LambertianEnvironmentBaker
{
public:
	LambertianEnvironmentBaker(IBLUtilsApplicationSpecification* specs_);

	void Run();

	void CreateComputePipeline();

	void Prepare();

	void ExecuteComputeShader();

	void SaveAsImage();

private:
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_Pipeline;

	std::shared_ptr<ImageCube> inputImg;
	std::shared_ptr<ImageCube> storageImg;

	DescriptorAllocator m_DescriptorAllocator;

	VkDescriptorSetLayout set1_TextureLayout = VK_NULL_HANDLE;
	VkDescriptorSet set1_Texture = VK_NULL_HANDLE;

	VkFence fence;

	IBLUtilsApplicationSpecification* specs;
};

