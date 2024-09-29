#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>

// Referenced from https://vkguide.dev/docs/extra-chapter/abstracting_descriptors/
class DescriptorLayoutCache {
public:
	void Cleanup();

	VkDescriptorSetLayout CreateDescriptorLayout(VkDescriptorSetLayoutCreateInfo* info);

	struct DescriptorLayoutInfo 
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		bool operator==(const DescriptorLayoutInfo& other) const;

		size_t hash() const;
	};

private:

	struct DescriptorLayoutHash {
		std::size_t operator()(const DescriptorLayoutInfo& k) const {
			return k.hash();
		}
	};

	std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> layoutCache;
};