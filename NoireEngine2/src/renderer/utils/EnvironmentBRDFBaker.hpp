#pragma once

#include "backend/VulkanContext.hpp"
#include "backend/images/Image2D.hpp"

struct IBLUtilsApplicationSpecification;

class EnvironmentBRDFBaker
{
public:
	EnvironmentBRDFBaker(IBLUtilsApplicationSpecification* specs_);

	~EnvironmentBRDFBaker();

	void Run();

	void Setup();

	void ExecuteComputeShader();

	void SaveAsImage();

private:
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_Pipeline;

	std::shared_ptr<Image2D> brdfImage;

	DescriptorAllocator m_DescriptorAllocator;

	VkDescriptorSetLayout set0_TextureLayout = VK_NULL_HANDLE;
	VkDescriptorSet set0_Textures;

	VkFence fence;

	IBLUtilsApplicationSpecification* specs;
};

