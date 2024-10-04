#include "VulkanContext.hpp"

#include <format>
#include <vulkan/vk_enum_string_helper.h>

#include "utils/Enumerate.hpp"
#include "utils/Logger.hpp"
#include "core/Timer.hpp"

#define MAX_DRAW_COMMANDS 1000000

VulkanContext::VulkanContext() :
    s_VulkanInstance(std::make_unique<VulkanInstance>()),
    s_PhysicalDevice(std::make_unique<PhysicalDevice>(*s_VulkanInstance)),
    s_LogicalDevice(std::make_unique<LogicalDevice>(*s_VulkanInstance, *s_PhysicalDevice))
{
    CreatePipelineCache();
}

VulkanContext::~VulkanContext()
{
    WaitForCommands();

    m_Swapchains.clear();

    vkDestroyPipelineCache(*s_LogicalDevice, m_PipelineCache, nullptr);
    m_CommandPools.clear();

    CleanPerSurfaceStructs();

    s_Renderer->Cleanup();

    std::cout << "Destroyed vulkan context module\n";
}

void VulkanContext::Update()
{
    s_Renderer->Update();

    for (auto [surfaceId, swapchain] : Enumerate(m_Swapchains))
    {
        Timer timer;

        auto& perSurfaceBuffer = m_PerSurfaceBuffers[surfaceId];

        VkResult acquireResult = swapchain->AcquireNextImage(
            perSurfaceBuffer->getPresentSemaphore(),
            perSurfaceBuffer->getFence()
        );

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
            RecreateSwapchain();
            return;
        }

        if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
            std::cerr << "[vulkan] Acquiring swapchain image resulted in " << string_VkResult(acquireResult) << std::endl;
            return;
        }

        std::unique_ptr<CommandBuffer>& commandBuffer = perSurfaceBuffer->commandBuffers[swapchain->getActiveImageIndex()];

        commandBuffer->Begin();

        if (Application::StatsDirty)
            WaitForSwapchainTime = timer.GetElapsed(true);

        s_Renderer->Render(*commandBuffer, static_cast<uint32_t>(surfaceId));
        // submit the command buffer
        {
            commandBuffer->End();
            commandBuffer->Submit(perSurfaceBuffer->getPresentSemaphore(),
                perSurfaceBuffer->getRenderSemaphore(),
                perSurfaceBuffer->getFence());

            auto presentResult = swapchain->QueuePresent(s_LogicalDevice->getPresentQueue(), perSurfaceBuffer->getRenderSemaphore());
            if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
                perSurfaceBuffer->framebufferResized = true;
            }
            else if (presentResult != VK_SUCCESS) {
                VK_CHECK(presentResult, "[vulkan] Failed to present swap chain image!");
            }
        }

        if (Application::StatsDirty)
            RenderTime = timer.GetElapsed(false);

        perSurfaceBuffer->currentFrame = (perSurfaceBuffer->currentFrame + 1) % swapchain->getImageCount();
    }
}

void VulkanContext::OnWindowResize(uint32_t width, uint32_t height)
{
    RecreateSwapchain();
}

void VulkanContext::OnAddWindow(Window* window)
{
    m_Surfaces.emplace_back(std::make_unique<Surface>(*s_VulkanInstance, *s_PhysicalDevice, *s_LogicalDevice, window));
    s_Renderer->CreateRenderPass();
    RecreateSwapchain();
    s_Renderer->CreatePipelines();
}

void VulkanContext::InitializeRenderer()
{
    s_Renderer = std::make_unique<Renderer>();
}

void VulkanContext::WaitForCommands()
{
    VK_CHECK(vkQueueWaitIdle(s_LogicalDevice->getGraphicsQueue()),
        "[vulkan] Error: Wait queue on destroy vulkan context failed");
}

uint32_t VulkanContext::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    auto& mems = VulkanContext::Get()->getPhysicalDevice()->getMemoryProperties();
    for (uint32_t i = 0; i < mems.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (mems.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("[vulkan] Error: failed to find suitable memory type!");
}

VkFormat VulkanContext::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(*(VulkanContext::Get()->getPhysicalDevice()), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("[vulkan] Error: failed to find supported format!");
}

void VulkanContext::VK_CHECK(VkResult err, const char* msg)
{
    if (err == VK_SUCCESS)
        return;

    NE_ERROR(std::format("[vulkan] Error: {} with errorno: {}", msg, string_VkResult(err)));
}

void VulkanContext::VK_CHECK(VkResult err)
{
    if (err == VK_SUCCESS)
        return;

    NE_ERROR("[vulkan] Error: ", string_VkResult(err));
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

void VulkanContext::RecreateSwapchain()
{
    WaitForCommands();

    auto wd = getSurface(0)->getWindow();
    uint32_t w, h;
    do {
        w = wd->m_Data.Width;
        h = wd->m_Data.Height;
    } while (!w || !h);
    auto extent = VkExtent2D{ w, h };

    CleanPerSurfaceStructs();

    m_Swapchains.resize(m_Surfaces.size());
    m_PerSurfaceBuffers.resize(m_Surfaces.size());

    for (const auto& [id, surface] : Enumerate(m_Surfaces)) 
    {
        m_Swapchains[id] = std::make_unique<SwapChain>(*s_PhysicalDevice, *surface, *s_LogicalDevice, extent, m_Swapchains[id].get());
        m_PerSurfaceBuffers[id] = std::make_unique<PerSurfaceBuffers>();
        
        auto& perSurfaceBuffer = m_PerSurfaceBuffers[id];

        // resize everything
        uint32_t img_cnt = m_Swapchains[id]->getImageCount();
        perSurfaceBuffer->presentCompletesSemaphores.resize(img_cnt);
        perSurfaceBuffer->renderCompletesSemaphores.resize(img_cnt);
        perSurfaceBuffer->flightFences.resize(img_cnt);
        perSurfaceBuffer->commandBuffers.resize(img_cnt);
        perSurfaceBuffer->drawIndirectBuffers.resize(img_cnt);

        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (std::size_t i = 0; i < img_cnt; ++i) {
            VK_CHECK(vkCreateSemaphore(*s_LogicalDevice, &semaphoreCreateInfo, nullptr, &perSurfaceBuffer->presentCompletesSemaphores[i]),
                "[vulkan] Error creating semaphores");

            VK_CHECK(vkCreateSemaphore(*s_LogicalDevice, &semaphoreCreateInfo, nullptr, &perSurfaceBuffer->renderCompletesSemaphores[i]),
                "[vulkan] Error creating semaphores");

            VK_CHECK(vkCreateFence(*s_LogicalDevice, &fenceCreateInfo, nullptr, &perSurfaceBuffer->flightFences[i]),
                "[vulkan] Error creating semaphores");

            perSurfaceBuffer->commandBuffers[i] = std::make_unique<CommandBuffer>(false);

            perSurfaceBuffer->drawIndirectBuffers[i] = std::make_unique<Buffer>(
                MAX_DRAW_COMMANDS * sizeof(VkDrawIndexedIndirectCommand),
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                Buffer::Mapped
            );
        }
    }

    s_Renderer->Rebuild();
}

void VulkanContext::CleanPerSurfaceStructs()
{
    for (const auto& [id, perSurfaceBuffer] : Enumerate(m_PerSurfaceBuffers))
    {
        for (std::size_t i = 0; i < perSurfaceBuffer->flightFences.size(); i++)
        {
            perSurfaceBuffer->drawIndirectBuffers[i]->Destroy();

            vkDestroyFence(*s_LogicalDevice, perSurfaceBuffer->flightFences[i], nullptr);
            vkDestroySemaphore(*s_LogicalDevice, perSurfaceBuffer->presentCompletesSemaphores[i], nullptr);
            vkDestroySemaphore(*s_LogicalDevice, perSurfaceBuffer->renderCompletesSemaphores[i], nullptr);

            perSurfaceBuffer->commandBuffers.clear();
        }
    }
}
