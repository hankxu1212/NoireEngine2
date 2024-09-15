#pragma once

#include <vulkan/vulkan.h>

class VulkanInstance;
class PhysicalDevice;

class LogicalDevice 
{
	friend class VulkanContext;
public:
	LogicalDevice(const VulkanInstance& m_Instance, const PhysicalDevice& m_PhysicalDevice);
	~LogicalDevice();

	operator const VkDevice& () const { return m_LogicalDevice; }

	const VkPhysicalDeviceFeatures& getEnabledFeatures() const { return m_EnabledFeatures; }
	const VkQueue&					getGraphicsQueue() const { return m_GraphicsQueue; }
	const VkQueue&					getPresentQueue() const { return m_PresentQueue; }
	const VkQueue&					getComputeQueue() const { return m_ComputeQueue; }
	const VkQueue&					getTransferQueue() const { return m_TransferQueue; }
	uint32_t						getGraphicsFamily() const { return m_GraphicsFamily; }
	uint32_t						getPresentFamily() const { return m_PresentFamily; }
	uint32_t						getComputeFamily() const { return m_ComputeFamily; }
	uint32_t						getTransferFamily() const { return m_TransferFamily; }

	static const std::vector<const char*> DeviceExtensions;

private:
	void CreateQueueIndices();
	void CreateLogicalDevice();

	const VulkanInstance&				m_Instance;
	const PhysicalDevice&				m_PhysicalDevice;

	VkDevice							m_LogicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceFeatures			m_EnabledFeatures = {};

	VkQueueFlags						m_SupportedQueues = {};
	uint32_t							m_GraphicsFamily = 0;
	uint32_t							m_PresentFamily = 0;
	uint32_t							m_ComputeFamily = 0;
	uint32_t							m_TransferFamily = 0;

	VkQueue								m_GraphicsQueue = VK_NULL_HANDLE;
	VkQueue								m_PresentQueue = VK_NULL_HANDLE;
	VkQueue								m_ComputeQueue = VK_NULL_HANDLE;
	VkQueue								m_TransferQueue = VK_NULL_HANDLE;
};
