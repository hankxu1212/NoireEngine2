#include "DescriptorBuilder.hpp"

#include "backend/VulkanContext.hpp"

DescriptorBuilder DescriptorBuilder::Start(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator) 
{
	DescriptorBuilder builder;

	builder.cache = layoutCache;
	builder.alloc = allocator;
	return builder;
}

DescriptorBuilder& DescriptorBuilder::BindBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags)
{
	//create the descriptor binding for the layout
	VkDescriptorSetLayoutBinding newBinding{};

	newBinding.descriptorCount = 1;
	newBinding.descriptorType = type;
	newBinding.pImmutableSamplers = nullptr;
	newBinding.stageFlags = stageFlags;
	newBinding.binding = binding;

	bindings.push_back(newBinding);

	//create the descriptor write
	VkWriteDescriptorSet newWrite{};
	newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	newWrite.pNext = nullptr;

	newWrite.descriptorCount = 1;
	newWrite.descriptorType = type;
	newWrite.pBufferInfo = bufferInfo;
	newWrite.dstBinding = binding;

	writes.push_back(newWrite);
	return *this;
}

DescriptorBuilder& DescriptorBuilder::BindImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, 
	VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t descriptorCount)
{
	bindings.emplace_back( VkDescriptorSetLayoutBinding {
		.binding = binding,
		.descriptorType = type,
		.descriptorCount = descriptorCount,
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr,
	});

	writes.emplace_back( VkWriteDescriptorSet {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstBinding = binding,
		.descriptorCount = descriptorCount,
		.descriptorType = type,
		.pImageInfo = imageInfo,
	});

	return *this;
}

DescriptorBuilder& DescriptorBuilder::AddBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags)
{
	//create the descriptor binding for the layout
	VkDescriptorSetLayoutBinding newBinding{};

	newBinding.descriptorCount = 1;
	newBinding.descriptorType = type;
	newBinding.pImmutableSamplers = nullptr;
	newBinding.stageFlags = stageFlags;
	newBinding.binding = binding;

	bindings.push_back(newBinding);

	return *this;
}

bool DescriptorBuilder::Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout, const void* pNextLayout, VkDescriptorSetLayoutCreateFlags flags, const void* pNextAlloc) {
	BuildLayout(layout, pNextLayout, flags);

	//allocate descriptor
	bool success = alloc->Allocate(&set, layout, pNextAlloc);
	if (!success) { return false; };

	//write descriptor
	Write(set);

	return true;
}

void DescriptorBuilder::BuildLayout(VkDescriptorSetLayout& layout, const void* pNext, VkDescriptorSetLayoutCreateFlags flags)
{
	VkDescriptorSetLayoutCreateInfo layoutInfo
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = pNext,
		.flags = flags,
		.bindingCount = (uint32_t)bindings.size(),
		.pBindings = bindings.data(),
	};

	layout = cache->CreateDescriptorLayout(&layoutInfo);
}

void DescriptorBuilder::Write(VkDescriptorSet& set)
{
	if (writes.empty())
		return;

	for (VkWriteDescriptorSet& w : writes) {
		w.dstSet = set;
	}

	vkUpdateDescriptorSets(VulkanContext::GetDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
}
