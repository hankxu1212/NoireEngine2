#pragma once

#include "backend/VulkanContext.hpp"
#include "backend/images/ImageCube.hpp"

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

