#include <optional>

#include "backend/VulkanContext.hpp"

const std::vector<const char*> LogicalDevice::DeviceExtensions = { 
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
	VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME, // dynamic vertex binding
	VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, // descriptor indexing
	VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME, // enables scalar buffers
	VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, // enable device addressing

#ifdef _NE_USE_RTX
	// ray tracing
	VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
	VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
	VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
	VK_KHR_SPIRV_1_4_EXTENSION_NAME, // Required for VK_KHR_ray_tracing_pipeline,
	VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME // Required by VK_KHR_spirv_1_4
#endif
};

LogicalDevice::LogicalDevice(const VulkanInstance& instance, const PhysicalDevice& physicalDevice) :
	m_Instance(instance),
	m_PhysicalDevice(physicalDevice) 
{
	CreateQueueIndices();
	CreateLogicalDevice();
}

LogicalDevice::~LogicalDevice() {
	VulkanContext::VK(vkDeviceWaitIdle(m_LogicalDevice), "[vulkan] Error: Wait idle on destroy vulkan context failed");
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
		NE_WARN("Selected GPU does not support wireframe pipelines!");
	}

	if (physicalDeviceFeatures.samplerAnisotropy)
		enabledFeatures.samplerAnisotropy = VK_TRUE;
	else
		NE_WARN("Selected GPU does not support sampler anisotropy!");

	if (physicalDeviceFeatures.textureCompressionBC)
		enabledFeatures.textureCompressionBC = VK_TRUE;
	else if (physicalDeviceFeatures.textureCompressionASTC_LDR)
		enabledFeatures.textureCompressionASTC_LDR = VK_TRUE;
	else if (physicalDeviceFeatures.textureCompressionETC2)
		enabledFeatures.textureCompressionETC2 = VK_TRUE;

	if (physicalDeviceFeatures.vertexPipelineStoresAndAtomics)
		enabledFeatures.vertexPipelineStoresAndAtomics = VK_TRUE;
	else
		NE_WARN("Selected GPU does not support vertex pipeline stores and atomics!");

	if (physicalDeviceFeatures.fragmentStoresAndAtomics)
		enabledFeatures.fragmentStoresAndAtomics = VK_TRUE;
	else
		NE_WARN("Selected GPU does not support fragment stores and atomics!");

	if (physicalDeviceFeatures.shaderStorageImageExtendedFormats)
		enabledFeatures.shaderStorageImageExtendedFormats = VK_TRUE;
	else
		NE_WARN("Selected GPU does not support shader storage extended formats!");

	if (physicalDeviceFeatures.shaderStorageImageWriteWithoutFormat)
		enabledFeatures.shaderStorageImageWriteWithoutFormat = VK_TRUE;
	else
		NE_WARN("Selected GPU does not support shader storage write without format!");

	//enabledFeatures.shaderClipDistance = VK_TRUE;
	//enabledFeatures.shaderCullDistance = VK_TRUE;

	if (physicalDeviceFeatures.geometryShader)
		enabledFeatures.geometryShader = VK_TRUE;
	else
		NE_WARN("Selected GPU does not support geometry shaders!");

	if (physicalDeviceFeatures.tessellationShader)
		enabledFeatures.tessellationShader = VK_TRUE;
	else
		NE_WARN("Selected GPU does not support tessellation shaders!");

	if (physicalDeviceFeatures.multiViewport)
		enabledFeatures.multiViewport = VK_TRUE;
	else
		NE_WARN("Selected GPU does not support multi viewports!");

	if (physicalDeviceFeatures.multiDrawIndirect)
		enabledFeatures.multiDrawIndirect = VK_TRUE;
	else
		NE_WARN("Selected GPU does not support multi draw indirect!");

	VkDeviceCreateInfo deviceCreateInfo = {};

	// enable dynamic vertex input state
	VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT dynamicVertexInputExt{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT,
		.vertexInputDynamicState = true
	};
	
	deviceCreateInfo.pNext = &dynamicVertexInputExt;

	// enable descriptor indexing feature ext
	VkPhysicalDeviceDescriptorIndexingFeaturesEXT physicalDeviceDescriptorIndexingFeatures
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.descriptorBindingVariableDescriptorCount = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE,
	};

	dynamicVertexInputExt.pNext = &physicalDeviceDescriptorIndexingFeatures;

#if (defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT))
	// SRS - on macOS set environment variable to configure MoltenVK for using Metal argument buffers (needed for descriptor indexing)
	//     - MoltenVK supports Metal argument buffers on macOS, iOS possible in future (see https://github.com/KhronosGroup/MoltenVK/issues/1651)
	setenv("MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", "1", 1);
#endif

	VkPhysicalDeviceSynchronization2Features sync2Ext{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
		.synchronization2 = true
	};

	physicalDeviceDescriptorIndexingFeatures.pNext = &sync2Ext;

	// enable device addressing
	VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddresFeatures{};
	enabledBufferDeviceAddresFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	enabledBufferDeviceAddresFeatures.bufferDeviceAddress = VK_TRUE;
	sync2Ext.pNext = &enabledBufferDeviceAddresFeatures;

#ifdef _NE_USE_RTX
	// add ray tracing features
	{
		VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{};
		enabledRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
		enabledBufferDeviceAddresFeatures.pNext = &enabledRayTracingPipelineFeatures;

		VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};
		enabledAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
		enabledRayTracingPipelineFeatures.pNext = &enabledAccelerationStructureFeatures;

		// add scalar buffer blockout extension
		{
			VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayoutFeatures{};
			scalarBlockLayoutFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES;
			scalarBlockLayoutFeatures.scalarBlockLayout = VK_TRUE; // Enable scalar block layout
			enabledAccelerationStructureFeatures.pNext = &scalarBlockLayoutFeatures;
		}
	}
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

	VulkanContext::VK(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_LogicalDevice), 
		"[vulkan] Error: cannot create physical device");

	vkGetDeviceQueue(m_LogicalDevice, m_GraphicsFamily, 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_LogicalDevice, m_PresentFamily, 0, &m_PresentQueue);
	vkGetDeviceQueue(m_LogicalDevice, m_ComputeFamily, 0, &m_ComputeQueue);
	vkGetDeviceQueue(m_LogicalDevice, m_TransferFamily, 0, &m_TransferQueue);
}
