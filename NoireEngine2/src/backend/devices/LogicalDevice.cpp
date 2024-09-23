#include <optional>

#include "backend/VulkanContext.hpp"

const std::vector<const char*> LogicalDevice::DeviceExtensions = { 
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
	VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME // dynamic vertex binding
};

LogicalDevice::LogicalDevice(const VulkanInstance& instance, const PhysicalDevice& physicalDevice) :
	m_Instance(instance),
	m_PhysicalDevice(physicalDevice) 
{
	CreateQueueIndices();
	CreateLogicalDevice();
}

LogicalDevice::~LogicalDevice() {
	VulkanContext::VK_CHECK(vkDeviceWaitIdle(m_LogicalDevice), "[vulkan] Error: Wait idle on destroy vulkan context failed");
	vkDestroyDevice(m_LogicalDevice, nullptr);
}

void LogicalDevice::CreateQueueIndices() 
{
	uint32_t deviceQueueFamilyPropertyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &deviceQueueFamilyPropertyCount, nullptr);
	std::vector<VkQueueFamilyProperties> deviceQueueFamilyProperties(deviceQueueFamilyPropertyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &deviceQueueFamilyPropertyCount, deviceQueueFamilyProperties.data());

	std::optional<uint32_t> graphicsFamily, presentFamily, computeFamily, transferFamily;

	for (uint32_t i = 0; i < deviceQueueFamilyPropertyCount; i++) {
		// Check for graphics support.
		if (deviceQueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			graphicsFamily = i;
			this->m_GraphicsFamily = i;
			m_SupportedQueues |= VK_QUEUE_GRAPHICS_BIT;
		}

		if (deviceQueueFamilyProperties[i].queueCount > 0 /*&& presentSupport*/) {
			presentFamily = i;
			this->m_PresentFamily = i;
		}

		// Check for compute support.
		if (deviceQueueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
			computeFamily = i;
			this->m_ComputeFamily = i;
			m_SupportedQueues |= VK_QUEUE_COMPUTE_BIT;
		}

		// Check for transfer support.
		if (deviceQueueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
			transferFamily = i;
			this->m_TransferFamily = i;
			m_SupportedQueues |= VK_QUEUE_TRANSFER_BIT;
		}

		if (graphicsFamily && presentFamily && computeFamily && transferFamily) {
			break;
		}
	}

	if (!graphicsFamily)
		throw std::runtime_error("Failed to find queue family supporting VK_QUEUE_GRAPHICS_BIT");
}

void LogicalDevice::CreateLogicalDevice() 
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	float queuePriorities[1] = { 0.0f };

	if (m_SupportedQueues & VK_QUEUE_GRAPHICS_BIT) {
		VkDeviceQueueCreateInfo graphicsQueueCreateInfo = {};
		graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		graphicsQueueCreateInfo.queueFamilyIndex = m_GraphicsFamily;
		graphicsQueueCreateInfo.queueCount = 1;
		graphicsQueueCreateInfo.pQueuePriorities = queuePriorities;
		queueCreateInfos.emplace_back(graphicsQueueCreateInfo);
	}
	else {
		m_GraphicsFamily = 0;
	}

	if (m_SupportedQueues & VK_QUEUE_COMPUTE_BIT && m_ComputeFamily != m_GraphicsFamily) {
		VkDeviceQueueCreateInfo computeQueueCreateInfo = {};
		computeQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		computeQueueCreateInfo.queueFamilyIndex = m_ComputeFamily;
		computeQueueCreateInfo.queueCount = 1;
		computeQueueCreateInfo.pQueuePriorities = queuePriorities;
		queueCreateInfos.emplace_back(computeQueueCreateInfo);
	}
	else {
		m_ComputeFamily = m_GraphicsFamily;
	}

	if (m_SupportedQueues & VK_QUEUE_TRANSFER_BIT 
			&& m_TransferFamily != m_GraphicsFamily 
			&& m_TransferFamily != m_ComputeFamily) {
		VkDeviceQueueCreateInfo transferQueueCreateInfo = {};
		transferQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		transferQueueCreateInfo.queueFamilyIndex = m_TransferFamily;
		transferQueueCreateInfo.queueCount = 1;
		transferQueueCreateInfo.pQueuePriorities = queuePriorities;
		queueCreateInfos.emplace_back(transferQueueCreateInfo);
	}
	else {
		m_TransferFamily = m_GraphicsFamily;
	}

	auto& physicalDeviceFeatures = m_PhysicalDevice.getFeatures();
	VkPhysicalDeviceFeatures enabledFeatures = {};

	// Enable sample rate shading filtering if supported.
	if (physicalDeviceFeatures.sampleRateShading)
		enabledFeatures.sampleRateShading = VK_TRUE;

	// Fill mode non solid is required for wireframe display.
	if (physicalDeviceFeatures.fillModeNonSolid) {
		enabledFeatures.fillModeNonSolid = VK_TRUE;

		// Wide lines must be present for line width > 1.0f.
		if (physicalDeviceFeatures.wideLines)
			enabledFeatures.wideLines = VK_TRUE;
	}
	else {
		std::cout << "Selected GPU does not support wireframe pipelines!";
	}

	if (physicalDeviceFeatures.samplerAnisotropy)
		enabledFeatures.samplerAnisotropy = VK_TRUE;
	else
		std::cout << "Selected GPU does not support sampler anisotropy!";

	if (physicalDeviceFeatures.textureCompressionBC)
		enabledFeatures.textureCompressionBC = VK_TRUE;
	else if (physicalDeviceFeatures.textureCompressionASTC_LDR)
		enabledFeatures.textureCompressionASTC_LDR = VK_TRUE;
	else if (physicalDeviceFeatures.textureCompressionETC2)
		enabledFeatures.textureCompressionETC2 = VK_TRUE;

	if (physicalDeviceFeatures.vertexPipelineStoresAndAtomics)
		enabledFeatures.vertexPipelineStoresAndAtomics = VK_TRUE;
	else
		std::cout << "Selected GPU does not support vertex pipeline stores and atomics!";

	if (physicalDeviceFeatures.fragmentStoresAndAtomics)
		enabledFeatures.fragmentStoresAndAtomics = VK_TRUE;
	else
		std::cout << "Selected GPU does not support fragment stores and atomics!";

	if (physicalDeviceFeatures.shaderStorageImageExtendedFormats)
		enabledFeatures.shaderStorageImageExtendedFormats = VK_TRUE;
	else
		std::cout << "Selected GPU does not support shader storage extended formats!";

	if (physicalDeviceFeatures.shaderStorageImageWriteWithoutFormat)
		enabledFeatures.shaderStorageImageWriteWithoutFormat = VK_TRUE;
	else
		std::cout << "Selected GPU does not support shader storage write without format!";

	//enabledFeatures.shaderClipDistance = VK_TRUE;
	//enabledFeatures.shaderCullDistance = VK_TRUE;

	if (physicalDeviceFeatures.geometryShader)
		enabledFeatures.geometryShader = VK_TRUE;
	else
		std::cout << "Selected GPU does not support geometry shaders!";

	if (physicalDeviceFeatures.tessellationShader)
		enabledFeatures.tessellationShader = VK_TRUE;
	else
		std::cout << "Selected GPU does not support tessellation shaders!";

	if (physicalDeviceFeatures.multiViewport)
		enabledFeatures.multiViewport = VK_TRUE;
	else
		std::cout << "Selected GPU does not support multi viewports!";
		
	VkDeviceCreateInfo deviceCreateInfo = {};

	// enable dynamic vertex input state
	VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT dynamicVertexInputExt{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT,
		.vertexInputDynamicState = true
	};
	
	deviceCreateInfo.pNext = &dynamicVertexInputExt;

	// add synchronization feature
#ifdef VK_VERSION_1_3
	VkPhysicalDeviceSynchronization2Features sync2Ext{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
		.synchronization2 = true
	};
	dynamicVertexInputExt.pNext = &sync2Ext;
#endif

	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	if (m_Instance.ValidationLayersEnabled) {
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VulkanInstance::ValidationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = VulkanInstance::ValidationLayers.data();
	}
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();
	deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
	VulkanContext::VK_CHECK(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_LogicalDevice), 
		"[vulkan] Error: cannot create physical device");

	vkGetDeviceQueue(m_LogicalDevice, m_GraphicsFamily, 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_LogicalDevice, m_PresentFamily, 0, &m_PresentQueue);
	vkGetDeviceQueue(m_LogicalDevice, m_ComputeFamily, 0, &m_ComputeQueue);
	vkGetDeviceQueue(m_LogicalDevice, m_TransferFamily, 0, &m_TransferQueue);
}
