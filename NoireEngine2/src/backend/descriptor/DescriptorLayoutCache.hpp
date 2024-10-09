#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>

class DescriptorLayoutCache {
public:
	void Cleanup();

	VkDescriptorSetLayout CreateDescriptorLayout(const VkDescriptorSetLayoutCreateInfo* info);
private:

	std::vector<VkDescriptorSetLayout> allLayouts;
};