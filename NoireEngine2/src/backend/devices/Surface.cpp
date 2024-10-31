#include "backend/VulkanContext.hpp"
#include "utils/Logger.hpp"

#include <vulkan/vk_enum_string_helper.h>

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

	VkFormat DefaultSurfaceFormat = VK_FORMAT_B8G8R8A8_SRGB;
	VkColorSpaceKHR DefaultColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

	if (surfaceFormatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
		m_Format.format = DefaultSurfaceFormat;
		m_Format.colorSpace = surfaceFormats[0].colorSpace;
	}

	bool found = false;
	for (auto& surfaceFormat : surfaceFormats) {
		if (surfaceFormat.format == DefaultSurfaceFormat && surfaceFormat.colorSpace == DefaultColorSpace) {
			m_Format.format = surfaceFormat.format;
			m_Format.colorSpace = surfaceFormat.colorSpace;
			found = true;
			break;
		}
	}

	// select the first available color format
	if (!found) {
		m_Format.format = surfaceFormats[0].format;
		m_Format.colorSpace = surfaceFormats[0].colorSpace;
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
