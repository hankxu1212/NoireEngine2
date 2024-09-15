#pragma once

#include <vulkan/vulkan.h>

class VulkanInstance;
class LogicalDevice;
class PhysicalDevice;
class Window;

class VulkanSurface 
{
	friend class VulkanContext;
public:
	VulkanSurface(const VulkanInstance& instance, const PhysicalDevice& physicalDevice, const LogicalDevice& logicalDevice, Window* window);
	~VulkanSurface();

	void UpdateCapabilities();

	operator const VkSurfaceKHR& () const { return m_Surface; }

	const VkSurfaceKHR&				getSurface() const { return m_Surface; }
	const VkSurfaceCapabilitiesKHR& getCapabilities() const { return m_Capabilities; }
	const VkSurfaceFormatKHR&		getFormat() const { return m_Format; }
	const Window*					getWindow() const { return m_Window; }

private:
	const Window*					m_Window;

	VkSurfaceKHR					m_Surface = VK_NULL_HANDLE;
	VkSurfaceCapabilitiesKHR		m_Capabilities = {};
	VkSurfaceFormatKHR				m_Format = {};
};
