#include "DescriptorAllocator.hpp"

#include "backend/VulkanContext.hpp"

// helper for creating a descriptor pool
static VkDescriptorPool CreatePool(VkDevice device, const DescriptorAllocator::PoolSizes& poolSizes, int count, VkDescriptorPoolCreateFlags flags)
{
	std::vector<VkDescriptorPoolSize> sizes;
	sizes.reserve(poolSizes.sizes.size());
	for (auto sz : poolSizes.sizes) {
		sizes.push_back({ sz.first, uint32_t(sz.second * count) });
	}
	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = flags;
	pool_info.maxSets = count;
	pool_info.poolSizeCount = (uint32_t)sizes.size();
	pool_info.pPoolSizes = sizes.data();

	VkDescriptorPool descriptorPool;
	vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool);

	return descriptorPool;
}

void DescriptorAllocator::Cleanup()
{
	//delete every pool held
	for (auto p : freePools)
	{
		vkDestroyDescriptorPool(VulkanContext::GetDevice(), p, nullptr);
	}
	for (auto p : usedPools)
	{
		vkDestroyDescriptorPool(VulkanContext::GetDevice(), p, nullptr);
	}
}

//On the function, we reuse a descriptor pool if it’s availible, 
// and if we don’t have one, we then create a new pool to hold 1000 descriptors. 
// The 1000 count is arbitrary. Could also create growing pools or different sizes.
VkDescriptorPool DescriptorAllocator::GrabPool()
{
	//there are reusable pools availible
	if (freePools.size() > 0)
	{
		//grab pool from the back of the vector and remove it from there.
		VkDescriptorPool pool = freePools.back();
		freePools.pop_back();
		return pool;
	}
	else
	{
		//no pools availible, so create a new one
		return CreatePool(VulkanContext::GetDevice(), descriptorSizes, 1000, 0);
	}
}

bool DescriptorAllocator::Allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout)
{
	//initialize the currentPool handle if it's null
	if (currentPool == VK_NULL_HANDLE) {

		currentPool = GrabPool();
		usedPools.push_back(currentPool);
	}

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;

	allocInfo.pSetLayouts = &layout;
	allocInfo.descriptorPool = currentPool;
	allocInfo.descriptorSetCount = 1;

	//try to allocate the descriptor set
	VkResult allocResult = vkAllocateDescriptorSets(VulkanContext::GetDevice(), &allocInfo, set);
	bool needReallocate = false;

	switch (allocResult) {
	case VK_SUCCESS:
		//all good, return
		return true;
	case VK_ERROR_FRAGMENTED_POOL:
	case VK_ERROR_OUT_OF_POOL_MEMORY:
		//reallocate pool
		needReallocate = true;
		break;
	default:
		//unrecoverable error
		return false;
	}

	if (needReallocate) {
		//allocate a new pool and retry
		currentPool = GrabPool();
		usedPools.push_back(currentPool);

		allocResult = vkAllocateDescriptorSets(VulkanContext::GetDevice(), &allocInfo, set);

		//if it still fails then we have big issues
		if (allocResult == VK_SUCCESS) {
			return true;
		}
	}

	return false;
}

void DescriptorAllocator::ResetPools() {
	//reset all used pools and add them to the free pools
	for (auto p : usedPools) {
		vkResetDescriptorPool(VulkanContext::GetDevice(), p, 0);
		freePools.push_back(p);
	}

	//clear the used pools, since we've put them all in the free pools
	usedPools.clear();

	//reset the current pool handle back to null
	currentPool = VK_NULL_HANDLE;
}