#include "VulkanContext.hpp"

#include <vulkan/vk_enum_string_helper.h>

VulkanContext::VulkanContext() :
    s_Instance(std::make_unique<VulkanInstance>()),
    s_PhysicalDevice(std::make_unique<PhysicalDevice>(*s_Instance)),
    s_LogicalDevice(std::make_unique<LogicalDevice>(*s_Instance, *s_PhysicalDevice))
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

std::shared_ptr<CommandPool>& VulkanContext::GetCommandPool(const TID& threadId)
{
    if (auto it = m_CommandPools.find(threadId); it != m_CommandPools.end())
        return it->second;
    return m_CommandPools.emplace(threadId, std::make_shared<CommandPool>(threadId)).first->second;
}

void VulkanContext::CreatePipelineCache()
{
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VK_CHECK(vkCreatePipelineCache(*s_LogicalDevice, &pipelineCacheCreateInfo, nullptr, &m_PipelineCache),
        "[vulkan] Error creating pipeline cache");
}
