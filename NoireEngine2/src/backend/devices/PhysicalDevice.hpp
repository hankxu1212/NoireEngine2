#pragma once

#include <vulkan/vulkan.h>

class VulkanInstance;

class PhysicalDevice 
{
	friend class VulkanContext;
public:
	explicit PhysicalDevice(const VulkanInstance& m_Instance);

	operator const VkPhysicalDevice& () const { return m_PhysicalDevice; }

	const VkPhysicalDevice&					getPhysicalDevice() const		{ return m_PhysicalDevice; }
	const VkPhysicalDeviceProperties&		getProperties() const			{ return m_Properties; }
	const VkPhysicalDeviceFeatures&			getFeatures() const				{ return m_Features; }
	const VkPhysicalDeviceMemoryProperties& getMemoryProperties() const		{ return m_MemoryProperties; }
	const VkSampleCountFlagBits&			getMsaaSamples() const			{ return m_MsaaSamples; }

private:
	VkSampleCountFlagBits					GetMaxUsableSampleCount() const;

	const VulkanInstance&				m_Instance;

	VkPhysicalDevice					m_PhysicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties			m_Properties = {};
	VkPhysicalDeviceFeatures			m_Features = {};
	VkPhysicalDeviceMemoryProperties	m_MemoryProperties = {};
	VkSampleCountFlagBits				m_MsaaSamples = VK_SAMPLE_COUNT_1_BIT;
};

