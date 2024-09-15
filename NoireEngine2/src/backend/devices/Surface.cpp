#include "backend/VulkanContext.hpp"

Surface::Surface(const VulkanInstance& instance, const PhysicalDevice& physicalDevice, const LogicalDevice& logicalDevice, Window* window) :
	m_Window(window)
{
	// Creates the glfw surface.
	VulkanContext::VK_CHECK(glfwCreateWindowSurface(instance, window->m_Window, nullptr, &m_Surface),
		"[vulkan] Error: Cannot create surface");

	VulkanContext::VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &m_Capabilities),
		"[vulkan] Error: cannot get surface capabilities.");

	uint32_t surfaceFormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &surfaceFormatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &surfaceFormatCount, surfaceFormats.data());

	if (surfaceFormatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
		m_Format.format = VK_FORMAT_B8G8R8A8_UNORM;
		m_Format.colorSpace = surfaceFormats[0].colorSpace;
	}
	else {
		// Iterate over the list of available surface format and
		// check for the presence of VK_FORMAT_B8G8R8A8_UNORM
		bool found_B8G8R8A8_UNORM = false;

		for (auto& surfaceFormat : surfaceFormats) {
			if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM) {
				m_Format.format = surfaceFormat.format;
				m_Format.colorSpace = surfaceFormat.colorSpace;
				found_B8G8R8A8_UNORM = true;
				break;
			}
		}

		// In case VK_FORMAT_B8G8R8A8_UNORM is not available
		// select the first available color format
		if (!found_B8G8R8A8_UNORM) {
			m_Format.format = surfaceFormats[0].format;
			m_Format.colorSpace = surfaceFormats[0].colorSpace;
		}
	}

	// Check for presentation support.
	VkBool32 presentSupport;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, logicalDevice.getPresentFamily(), m_Surface, &presentSupport);

	assert(presentSupport && "[vulkan] Error: Present queue family does not have presentation support");
}

Surface::~Surface() 
{
	vkDestroySurfaceKHR(*(VulkanContext::Get()->getInstance()), m_Surface, nullptr);
}

void Surface::UpdateCapabilities()
{
	VulkanContext::VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*(VulkanContext::Get()->getPhysicalDevice()), m_Surface, &m_Capabilities),
		"[vulkan] Error: cannot get surface capabilities.");
}
