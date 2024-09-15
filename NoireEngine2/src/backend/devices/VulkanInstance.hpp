#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class VulkanInstance {
public:
	VulkanInstance();
	~VulkanInstance();

	operator const VkInstance& () const { return m_Instance; }

	const VkInstance& getInstance() const { return m_Instance; }

	static const std::vector<const char*>  ValidationLayers;
	bool ValidationLayersEnabled = true;
private:
	VkDebugUtilsMessengerEXT		m_DebugMessenger = VK_NULL_HANDLE;
	VkInstance						m_Instance = VK_NULL_HANDLE;
};