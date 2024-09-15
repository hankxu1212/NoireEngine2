#include "VulkanContext.hpp"

#include <vulkan/vk_enum_string_helper.h>
#include <format>

VulkanContext::VulkanContext()
{
}

VulkanContext::~VulkanContext()
{
}

void VulkanContext::Update()
{
}

void VulkanContext::OnWindowResize(uint32_t width, uint32_t height)
{
}

void VulkanContext::OnAddWindow(Window* window)
{
}

void VulkanContext::WaitForCommands()
{
}

void VulkanContext::VK_CHECK(VkResult err, const char* msg)
{
    if (err == VK_SUCCESS)
        return;

    std::runtime_error(std::format("[vulkan] Error: {}, with err {}", msg, string_VkResult(err)));
}