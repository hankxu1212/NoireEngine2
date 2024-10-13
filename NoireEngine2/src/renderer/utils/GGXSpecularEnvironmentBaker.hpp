#pragma once

#include "backend/VulkanContext.hpp"
#include "backend/images/ImageCube.hpp"

// Epic uses 6 levels in its GGX pyramids, with 
// roughness = level / 5 (so level 0 is roughness 0 is the original image, 
// level 1 is roughness 1/5 ..., level 6 is roughness 1
#define GGX_MIP_LEVELS 6

struct IBLUtilsApplicationSpecification;

class GGXSpecularEnvironmentBaker
{
public:
	GGXSpecularEnvironmentBaker(IBLUtilsApplicationSpecification* specs_);

	void Run();

	void Setup();

	void Cleanup();

	void CreateComputePipeline(glm::uvec2 dim);

	void Prepare();

	void ExecuteComputeShader();

	void SaveAsImage(glm::uvec2 dim);

private:
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_Pipeline;

	std::shared_ptr<ImageCube> inputImg;
	std::shared_ptr<ImageCube> storageImg;

	DescriptorAllocator m_DescriptorAllocator;

	VkDescriptorSetLayout set0_TextureLayout = VK_NULL_HANDLE;
	VkDescriptorSet set0_Textures;

	VkFence fence;

	IBLUtilsApplicationSpecification* specs;

	int miplevel = 0;
};

