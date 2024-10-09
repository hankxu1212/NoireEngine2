#include "DescriptorLayoutCache.hpp"

#include "backend/VulkanContext.hpp"

void DescriptorLayoutCache::Cleanup() {
	//delete every descriptor layout held
	for (auto layout : allLayouts) {
		vkDestroyDescriptorSetLayout(VulkanContext::GetDevice(), layout, nullptr);
	}
}

VkDescriptorSetLayout DescriptorLayoutCache::CreateDescriptorLayout(const VkDescriptorSetLayoutCreateInfo* info) {
	VkDescriptorSetLayout layout;
	vkCreateDescriptorSetLayout(VulkanContext::GetDevice(), info, nullptr, &layout);
	allLayouts.push_back(layout);
	return layout;
}