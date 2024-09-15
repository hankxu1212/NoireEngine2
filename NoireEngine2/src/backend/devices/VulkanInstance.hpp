#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "backend/VulkanContext.hpp"

class VulkanInstance {
public:
	VulkanInstance();
	~VulkanInstance();

	operator const VkInstance& () const { return m_Instance; }

	bool getEnableValidationLayers() const { return m_EnableValidationLayers; }
	const VkInstance& getInstance() const { return m_Instance; }

	static const std::vector<const char*>  ValidationLayers;
private:
	void CreateInstance(VkDebugUtilsMessengerCreateInfoEXT& debug_create_info);

	bool m_EnableValidationLayers = true;
	VkDebugUtilsMessengerEXT		m_DebugMessenger = VK_NULL_HANDLE;
	VkInstance						m_Instance = VK_NULL_HANDLE;
};