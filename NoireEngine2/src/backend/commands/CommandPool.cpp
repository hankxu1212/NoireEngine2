#include "backend/VulkanContext.hpp"

CommandPool::CommandPool(const TID& threadId) :
	threadId(threadId) 
{
	VkCommandPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = VulkanContext::Get()->getLogicalDevice()->getGraphicsFamily(),
	};

	VulkanContext::VK(vkCreateCommandPool(VulkanContext::GetDevice(), &poolInfo, nullptr, &m_CommandPool),
		"[vulkan] Error: cannot create command pool");
}

CommandPool::~CommandPool() {
	vkDestroyCommandPool(VulkanContext::GetDevice(), m_CommandPool, nullptr);
}
