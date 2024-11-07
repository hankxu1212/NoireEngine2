#include "VulkanContext.hpp"

#include <atomic>
#include "utils/Enumerate.hpp"

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
    WaitGraphicsQueue();

    m_Swapchains.clear();

    vkDestroyPipelineCache(*s_LogicalDevice, m_PipelineCache, nullptr);
    m_CommandPools.clear();

    DestroyPerSurfaceStructs();

    m_DescriptorLayoutCache.Cleanup();

    if (s_Renderer)
        s_Renderer.reset();
}

void VulkanContext::LateInitialize()
{
    // create a default image so texture descriptors dont segfault
    Image2D::Create(Files::Path("../textures/default.png"), VK_FORMAT_R8G8B8A8_SRGB, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, false, false, true);

    s_Renderer = std::make_unique<Renderer>();
}

void VulkanContext::Update()
{
    s_Renderer->Update();

    for (auto [surfaceId, swapchain] : Enumerate(m_Swapchains))
    {
        auto& perSurfaceBuffer = m_PerSurfaceBuffers[surfaceId];

        VkResult acquireResult = swapchain->AcquireNextImage(
            perSurfaceBuffer->getPresentSemaphore(),
            perSurfaceBuffer->getFence()
        );

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
            RecreateSwapchain();
            return;
        }

        if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
            NE_ERROR("[vulkan] Acquiring swapchain image resulted in ", string_VkResult(acquireResult));

        std::unique_ptr<CommandBuffer>& commandBuffer = perSurfaceBuffer->commandBuffers[swapchain->getActiveImageIndex()];

        commandBuffer->Begin();

        s_Renderer->Render(*commandBuffer);

        // submit the command buffer
        commandBuffer->Submit(perSurfaceBuffer->getPresentSemaphore(),
            perSurfaceBuffer->getRenderSemaphore(),
            perSurfaceBuffer->getFence());

        // queue present
        auto presentResult = swapchain->QueuePresent(s_LogicalDevice->getPresentQueue(), perSurfaceBuffer->getRenderSemaphore());
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
            RecreateSwapchain();
        }
        VK(presentResult/*, "[vulkan] Failed to present swap chain image!"*/);

        perSurfaceBuffer->currentFrame = (perSurfaceBuffer->currentFrame + 1) % swapchain->getImageCount();
    }
}

void VulkanContext::OnWindowResize()
{
    RecreateSwapchain();
}

void VulkanContext::OnAddWindow(Window* window)
{
    // create a new surface
    m_Surfaces.emplace_back(std::make_unique<Surface>(*s_VulkanInstance, *s_PhysicalDevice, *s_LogicalDevice, window));

    s_Renderer->CreateRenderPass();
    RecreateSwapchain();
    s_Renderer->Create();
}

void VulkanContext::WaitGraphicsQueue()
{
    VK(vkQueueWaitIdle(s_LogicalDevice->getGraphicsQueue()));
}

uint32_t VulkanContext::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    auto& mems = VulkanContext::Get()->getPhysicalDevice()->getMemoryProperties();
    for (uint32_t i = 0; i < mems.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (mems.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    NE_ERROR("[vulkan] Error: failed to find suitable memory type!");
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

    NE_ERROR("[vulkan] Error: failed to find supported format!");
}

void VulkanContext::VK(VkResult err, const char* msg)
{
    if (err == VK_SUCCESS)
        return;

    NE_ERROR(std::format("[vulkan] Error: {} with errorno: {}", msg, string_VkResult(err)));
}

void VulkanContext::VK(VkResult err)
{
    if (err == VK_SUCCESS)
        return;

    NE_ERROR("[vulkan] Error: ", string_VkResult(err));
}

std::atomic_flag lock = ATOMIC_FLAG_INIT;

std::shared_ptr<CommandPool> VulkanContext::GetCommandPool(const TID& threadId)
{
    while (lock.test_and_set(std::memory_order_acquire)) {} // Acquire lock

    std::shared_ptr<CommandPool> ret;

    if (auto it = m_CommandPools.find(threadId); it != m_CommandPools.end())
        ret = it->second;
    else
        ret = m_CommandPools.emplace(threadId, std::make_shared<CommandPool>(threadId)).first->second;

    lock.clear(std::memory_order_release); // Release lock

    return ret;
}

void VulkanContext::CreatePipelineCache()
{
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
    VK(vkCreatePipelineCache(*s_LogicalDevice, &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));
}

void VulkanContext::RecreateSwapchain()
{
    WaitGraphicsQueue();

    auto wd = getSurface(0)->getWindow();
    uint32_t w, h;
    do {
        w = wd->m_Data.Width;
        h = wd->m_Data.Height;
    } while (!w || !h);

    VkExtent2D extent{ w, h };

    DestroyPerSurfaceStructs();

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
            VK(vkCreateSemaphore(*s_LogicalDevice, &semaphoreCreateInfo, nullptr, &perSurfaceBuffer->presentCompletesSemaphores[i]));
            VK(vkCreateSemaphore(*s_LogicalDevice, &semaphoreCreateInfo, nullptr, &perSurfaceBuffer->renderCompletesSemaphores[i]));
            VK(vkCreateFence(*s_LogicalDevice, &fenceCreateInfo, nullptr, &perSurfaceBuffer->flightFences[i]));

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

void VulkanContext::DestroyPerSurfaceStructs()
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
