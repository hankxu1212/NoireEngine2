#include <iomanip>
#include <set>
#include <sstream>

#include "backend/VulkanContext.hpp"
#include "utils/Logger.hpp"

static void LogVulkanDevice(const VkPhysicalDeviceProperties& physicalDeviceProperties, const std::vector<VkExtensionProperties>& extensionProperties) 
{
    std::stringstream ss;
    switch (static_cast<int32_t>(physicalDeviceProperties.deviceType)) {
    case 1:
        ss << "Integrated";
        break;
    case 2:
        ss << "Discrete";
        break;
    case 3:
        ss << "Virtual";
        break;
    case 4:
        ss << "CPU";
        break;
    default:
        ss << "Other " << physicalDeviceProperties.deviceType;
    }

    ss << " Physical Device: " << physicalDeviceProperties.deviceID;
    switch (physicalDeviceProperties.vendorID) {
    case 0x8086:
        ss << " \"Intel\"";
        break;
    case 0x10DE:
        ss << " \"Nvidia\"";
        break;
    case 0x1002:
        ss << " \"AMD\"";
        break;
    default:
        ss << " \"" << physicalDeviceProperties.vendorID << '\"';
    }

    ss << " " << std::quoted(physicalDeviceProperties.deviceName) << '\n';

    uint32_t supportedVersion[3] = {
        VK_VERSION_MAJOR(physicalDeviceProperties.apiVersion),
        VK_VERSION_MINOR(physicalDeviceProperties.apiVersion),
        VK_VERSION_PATCH(physicalDeviceProperties.apiVersion)
    };
    ss << "API Version: " << supportedVersion[0] << "." << supportedVersion[1] << "." << supportedVersion[2] << '\n';

#ifdef VK_VERBOSE_CALLBACK
    ss << "Extensions: ";
    for (const auto& extension : extensionProperties)
        ss << extension.extensionName << ",\n";
#endif
    ss << "\n";

    Logger::INFO(ss.str());
}

static int RateDeviceSuitability(VkPhysicalDevice device) {
    // query device extensions
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(LogicalDevice::DeviceExtensions.begin(), LogicalDevice::DeviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    if (!requiredExtensions.empty())
    {
        std::cout << "These extentions were not found: ";
        for (const auto& remainingExtensions : requiredExtensions)
            std::cout << remainingExtensions << ", ";

        std::cout << std::endl;
        return 0;
    }

    // query device props
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    LogVulkanDevice(deviceProperties, availableExtensions);

    // computes a scoring: discrete GPUs have a significant performance advantage
    int score = 0;
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;
    return score;
}

PhysicalDevice::PhysicalDevice(const VulkanInstance& m_Instance) :
    m_Instance(m_Instance)
{
    uint32_t gpu_count;
    VulkanContext::VK_CHECK(vkEnumeratePhysicalDevices(m_Instance, &gpu_count, nullptr), 
        "[vulkan] Error: cannot enumerate physical devices");
    assert(gpu_count > 0 && "[vulkan] Error: gpu count = 0");

    std::vector<VkPhysicalDevice> gpus;
    gpus.resize(gpu_count);
    VulkanContext::VK_CHECK(vkEnumeratePhysicalDevices(m_Instance, &gpu_count, gpus.data()),
        "[vulkan] Error: cannot enumerate gpus");

    // Use an ordered map to automatically sort candidates by increasing score
    std::multimap<int, VkPhysicalDevice> candidates;

    for (const auto& device : gpus)
        candidates.insert({ RateDeviceSuitability(device), device });

    // Check if the best candidate is suitable at all
    if (candidates.rbegin()->first > 0) {
        m_PhysicalDevice = candidates.rbegin()->second;

        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_Properties);
        vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_Features);
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_MemoryProperties);
        m_MsaaSamples = GetMaxUsableSampleCount();
    }
    else {
        m_PhysicalDevice = VK_NULL_HANDLE;
        throw std::runtime_error("[vulkan] Error: Failed to find a suitable physical device");
    }
}

VkSampleCountFlagBits PhysicalDevice::GetMaxUsableSampleCount() const 
{
    VkSampleCountFlags counts = m_Properties.limits.framebufferColorSampleCounts & m_Properties.limits.framebufferDepthSampleCounts;

    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}
