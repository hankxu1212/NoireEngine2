#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "DescriptorAllocator.hpp"
#include "DescriptorLayoutCache.hpp"

// Referenced from https://vkguide.dev/docs/extra-chapter/abstracting_descriptors/
// Creates a factory builder pattern for constructing descriptors 
class DescriptorBuilder 
{
public:
	static DescriptorBuilder Start(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator);

	DescriptorBuilder& BindBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);
	DescriptorBuilder& BindImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t descriptorCount=1);
	DescriptorBuilder& BindAccelerationStructure(uint32_t binding, VkWriteDescriptorSetAccelerationStructureKHR as, VkShaderStageFlags stageFlags);
	DescriptorBuilder& AddBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags);

	bool Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout, const void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags=0, const void* pNextAlloc = nullptr);
	void BuildLayout(VkDescriptorSetLayout& layout, const void* pNext=nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
	void Write(VkDescriptorSet& set);

private:

	std::vector<VkWriteDescriptorSet> writes;
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	DescriptorLayoutCache* cache;
	DescriptorAllocator* alloc;
};