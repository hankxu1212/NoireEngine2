#pragma once

#include <vulkan/vulkan.h>
#include <vector>

// Referenced from https://vkguide.dev/docs/extra-chapter/abstracting_descriptors/
class DescriptorAllocator 
{
public:

	void ResetPools();

	bool Allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout, const void* pNext=nullptr);

	void Cleanup();

	VkDescriptorPool GrabPool();

	void SetCustomPoolParams(std::vector<VkDescriptorPoolSize>& sizes, uint32_t maxSets_);

private:
	VkDescriptorPool currentPool{ VK_NULL_HANDLE };
	uint32_t maxSets = 1000;

	std::vector<VkDescriptorPoolSize> descriptorSizes =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 },
#ifdef _NE_USE_RTX
		{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 10 }
#endif
	};

	std::vector<VkDescriptorPool> usedPools;
	std::vector<VkDescriptorPool> freePools;
};