#include "backend/VulkanContext.hpp"
#include "utils/Logger.hpp"

static bool CheckValidationLayerSupport(const std::vector<const char*>& g_ValidationLayers) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : g_ValidationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

// validation layer callback with the `PFN_vkDebugUtilsMessengerCallbackEXT` prototype
// specifies what errors from Vulkan we print to output stream
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        Logger::INFO(pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        Logger::WARN(pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        Logger::ERROR(pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
#ifdef VK_VERBOSE_CALLBACK
        std::cout << "[vulkan] " << pCallbackData->pMessage;
#endif
        break;
    default:
        break;
    }

    return VK_FALSE;
}

// look up vkCreateDebugUtilsMessengerEXT address using vkGetInstanceProcAddr, 
// which can be called to create the VkDebugUtilsMessengerEXT object
static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance m_Instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(m_Instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void DestroyDebugUtilsMessengerEXT(
    VkInstance m_Instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(m_Instance, debugMessenger, pAllocator);
    }
}

static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

static bool IsExtensionAvailable(const std::vector<VkExtensionProperties>& properties, const char* extension)
{
    for (const VkExtensionProperties& p : properties)
        if (strcmp(p.extensionName, extension) == 0)
            return true;
    return false;
}


const std::vector<const char*> VulkanInstance::ValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
}; // "VK_LAYER_RENDERDOC_Capture"

VulkanInstance::VulkanInstance()
{
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Noire Engine Application";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Noire Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    // Instance Info
    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &app_info;

    // Validation Layers
    if (ValidationLayersEnabled && !CheckValidationLayerSupport(ValidationLayers))
        throw std::runtime_error("[vulkan] Error: Failed validation layer support check");

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info {};
    if (ValidationLayersEnabled) {
        instance_create_info.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
        instance_create_info.ppEnabledLayerNames = ValidationLayers.data();

        PopulateDebugMessengerCreateInfo(debug_create_info);
        instance_create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
    }
    else {
        instance_create_info.enabledLayerCount = 0;
        instance_create_info.pNext = nullptr;
    }

    // Required Extensions
    std::vector<const char*> extensions;
    uint32_t extensions_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
    for (uint32_t i = 0; i < extensions_count; i++)
        extensions.push_back(glfw_extensions[i]);

    // Enumerate available extensions
    uint32_t properties_count;
    std::vector<VkExtensionProperties> properties;
    vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
    properties.resize(properties_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.data());

    // Enable required extensions (portability and some MoltenSDK compatability stuff)
    if (ValidationLayersEnabled) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    if (IsExtensionAvailable(properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    else
        std::runtime_error("no VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME extension found possible on this device. Aborting...\n");

    if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
    {
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        instance_create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }

    // Create Vulkan Instance
    instance_create_info.enabledExtensionCount = (uint32_t)extensions.size();
    instance_create_info.ppEnabledExtensionNames = extensions.data();
    VulkanContext::VK_CHECK(vkCreateInstance(&instance_create_info, nullptr, &m_Instance),
        "[vulkan] Error: cannot create instance");

    if (ValidationLayersEnabled) {
        VulkanContext::VK_CHECK(CreateDebugUtilsMessengerEXT(m_Instance, &debug_create_info, nullptr, &m_DebugMessenger),
            "[vulkan] Error: cannot create debug messenger");
    }
}

VulkanInstance::~VulkanInstance()
{
    if (ValidationLayersEnabled) {
        DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
    }
    vkDestroyInstance(m_Instance, nullptr);
}