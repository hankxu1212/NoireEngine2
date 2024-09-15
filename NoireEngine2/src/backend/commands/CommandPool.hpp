#pragma once

#include <vulkan/vulkan.h>
#include "core/Core.hpp"

class CommandPool 
{
public:
	explicit CommandPool(const TID& threadId = std::this_thread::get_id());

	~CommandPool();

	operator const VkCommandPool& () const { return m_CommandPool; }
	const VkCommandPool& getCommandPool() const { return m_CommandPool; }

	const TID& getThreadId() const { return threadId; }
private:
	VkCommandPool		m_CommandPool = VK_NULL_HANDLE;
	TID					threadId;
};
