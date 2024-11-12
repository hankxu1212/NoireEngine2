#include "BloomPipeline.hpp"
#include "backend/VulkanContext.hpp"
#include "VulkanGraphicsPipelineBuilder.hpp"

#define HDR_FORMAT VK_FORMAT_R16G16B16A16_SFLOAT

BloomPipeline::~BloomPipeline()
{
    // Clean up bloom pipelines
    if (m_BloomPipelineUp != VK_NULL_HANDLE) {
        vkDestroyPipeline(VulkanContext::GetDevice(), m_BloomPipelineUp, nullptr);
        m_BloomPipelineUp = VK_NULL_HANDLE;
    }

    if (m_BloomPipelineDown != VK_NULL_HANDLE) {
        vkDestroyPipeline(VulkanContext::GetDevice(), m_BloomPipelineDown, nullptr);
        m_BloomPipelineDown = VK_NULL_HANDLE;
    }

    // Clean up bloom pipeline layout
    if (m_BloomPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(VulkanContext::GetDevice(), m_BloomPipelineLayout, nullptr);
        m_BloomPipelineLayout = VK_NULL_HANDLE;
    }

    DestroyWorkspaces();

    m_DescriptorAllocator.Cleanup();
}

void BloomPipeline::CreateRenderPass()
{
    s_RenderPassDown = std::make_unique<Renderpass>();
    s_RenderPassUp = std::make_unique<Renderpass>();
    s_RenderPassDown->SetClearValues({
        {.color = { { 0.0f, 0.0f, 0.0f, 1.0f } } },  // Clear color to black
    });
    s_RenderPassUp->SetClearValues({
        {.color = { { 0.0f, 0.0f, 0.0f, 1.0f } } },  // Clear color to black
    });

    for (Workspace& workspace : workspaces)
    {
        CreateImageViews(workspace);
        CreateAttachments(workspace);
        CreateDescriptors(workspace);
    }

    CreateRenderPasses();

    for (Workspace& workspace : workspaces)
    {
        CreateFrameBuffers(workspace);
    }
}

void BloomPipeline::Rebuild()
{
    DestroyWorkspaces();
}

void BloomPipeline::InitializeWorkspaces()
{
    workspaces.resize(VulkanContext::Get()->getFramesInFlight());
}

void BloomPipeline::OnUIRender()
{
    ImGui::DragFloat("Filter Radius", &m_BloomPush.filterRadius, 0.01f, 0.0f, 1.0f, "%.4f");
}

void BloomPipeline::CreatePipeline()
{
    CreateBloomPipelineLayout();
    CreateBloomPipelines();
}

void BloomPipeline::Render(const Scene* scene, const CommandBuffer& commandBuffer)
{
    Workspace& workspace = workspaces[CURR_FRAME];

    const SwapChain* swapchain = VulkanContext::Get()->getSwapChain();
    VkExtent2D swapchainExtent = swapchain->getExtent();

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_BloomPipelineLayout,            // VkPipelineLayout layout
        0,                                // uint32_t         firstSet
        1,                                // uint32_t         descriptorSetCount
        &workspace.set0_MipViews, // VkDescriptorSet* pDescriptorSets
        0,                                // uint32_t         dynamicOffsetCount
        nullptr                           // const uint32_t*  pDynamicOffsets
    );

    // down -------------------------------------------------------------------------------------------------------------
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_BloomPipelineDown);

    // sample from mip level 0 to mip level NUMBER_OF_MIPMAPS - 2
    // render into mip level 1 to mip level NUMBER_OF_MIPMAPS - 1
    // e.g. if NUMBER_OF_MIPMAPS == 4, then sample from 0, 1, 2 and render into 1, 2, 3
    int mipLevel = 0;
    for (int index = 0; index < BLOOM_N_DOWNSAMPLED_IMGS; ++index)
    {
        VkExtent2D extent{ swapchainExtent.width >> (mipLevel + 1), swapchainExtent.height >> (mipLevel + 1) };
        s_RenderPassDown->Begin(commandBuffer, workspace.downFB[index], extent);
        
        m_BloomPush.texelSize = glm::vec2(1 / extent.width, 1 / extent.height);
        m_BloomPush.mipLevel = mipLevel;

        vkCmdPushConstants(commandBuffer, m_BloomPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
            sizeof(BloomPush), &m_BloomPush);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        s_RenderPassDown->End(commandBuffer);

        mipLevel++;
    }

    // up ---------------------------------------------------------------------------------------------------------------
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_BloomPipelineUp);

    // sample from mip level mip level NUMBER_OF_MIPMAPS - 1 to 1
    // render into mip level NUMBER_OF_MIPMAPS - 2 to mip level 0
    // e.g. if NUMBER_OF_MIPMAPS == 4, then sample from 3, 2, 1 and render into 2, 1, 0
    assert(mipLevel == BLOOM_N_DOWNSAMPLED_IMGS);

    for (int index = 0; index < BLOOM_N_DOWNSAMPLED_IMGS; ++index)
    {
        VkExtent2D extent{ swapchainExtent.width >> (mipLevel - 1), swapchainExtent.height >> (mipLevel - 1) };
        s_RenderPassUp->Begin(commandBuffer, workspace.upFB[index], extent);

        // texel size not used in upper pass
        //m_BloomPush.texelSize = glm::vec2(1 / extent.width, 1 / extent.height);
        m_BloomPush.mipLevel = mipLevel;

        vkCmdPushConstants(commandBuffer, m_BloomPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
            sizeof(BloomPush), &m_BloomPush);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        s_RenderPassUp->End(commandBuffer);
        mipLevel--;
    }
}

void BloomPipeline::CreateImageViews(Workspace& workspace)
{
	for (int mipLevel = 0; mipLevel < BLOOM_MIP_LEVELS; ++mipLevel)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.pNext = nullptr;
		viewInfo.flags = 0;
        viewInfo.image = workspace.bloomImage->getImage();
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = HDR_FORMAT;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = mipLevel;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		vkCreateImageView(VulkanContext::GetDevice(), &viewInfo, nullptr, &workspace.m_BloomImageViews[mipLevel]);
	}
}

void BloomPipeline::CreateAttachments(Workspace& workspace)
{
    VkExtent2D extent = VulkanContext::Get()->getSwapChain()->getExtent();
    { // down
           // iterate from mip 1 (first image to down sample into) to the last mip;
           // the level 1 mip image and following mip levels have to be cleared
           //
           //  --> VK_ATTACHMENT_LOAD_OP_CLEAR
           //
           // e.g. if BLOOM_MIP_LEVELS == 4, then use mip level 1, 2, 3
           // so that we downsample mip 0 into mip 1 (== render target), etc.
           // (the g-buffer level zero image must not be cleared)
           // before the pass: VK_IMAGE_LAYOUT_UNDEFINED
           // after the pass VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
           // during the pass: VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        int mipLevel = 1;
        for (int index = 0; index < BLOOM_N_DOWNSAMPLED_IMGS; ++index)
        {
            VkExtent2D extentMipLevel{ extent.width >> mipLevel, extent.height >> mipLevel };

            Attachment attachment{
                workspace.m_BloomImageViews[mipLevel],          // VkImageView         m_ImageView;
                HDR_FORMAT,                                   // VkFormat            m_Format;
                extentMipLevel,                           // VkExtent2D          m_Extent;
                VK_ATTACHMENT_LOAD_OP_CLEAR,              // VkAttachmentLoadOp  m_LoadOp;
                VK_ATTACHMENT_STORE_OP_STORE,             // VkAttachmentStoreOp m_StoreOp;
                VK_IMAGE_LAYOUT_GENERAL,                // VkImageLayout       m_InitialLayout;
                VK_IMAGE_LAYOUT_GENERAL, // VkImageLayout       m_FinalLayout;
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL  // VkImageLayout       m_SubpassLayout;
            };
            workspace.downAttachments.push_back(attachment);

            ++mipLevel;
        }
    }

    { // up
        // iterate from second last mip to mip 0;
        // do not clear the images
        //
        //  --> VK_ATTACHMENT_LOAD_OP_LOAD
        //
        // e.g. if BLOOM_MIP_LEVELS == 4, then use mip level 2, 1, 0
        // so that we upsample the last mip (mip BLOOM_MIP_LEVELS-1) into (mip BLOOM_MIP_LEVELS-2)
        // before the pass: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        // after the pass VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        // during the pass: VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        int mipLevel = BLOOM_N_DOWNSAMPLED_IMGS - 1;
        for (int index = 0; index < BLOOM_N_DOWNSAMPLED_IMGS; ++index)
        {
            VkExtent2D extentMipLevel{ extent.width >> mipLevel, extent.height >> mipLevel };

            Attachment attachment{
                workspace.m_BloomImageViews[mipLevel],          // VkImageView         m_ImageView;
                HDR_FORMAT,                                   // VkFormat            m_Format;
                extentMipLevel,                           // VkExtent2D          m_Extent;
                VK_ATTACHMENT_LOAD_OP_LOAD,               // VkAttachmentLoadOp  m_LoadOp;
                VK_ATTACHMENT_STORE_OP_STORE,             // VkAttachmentStoreOp m_StoreOp;
                VK_IMAGE_LAYOUT_GENERAL, // VkImageLayout       m_InitialLayout;
                VK_IMAGE_LAYOUT_GENERAL, // VkImageLayout       m_FinalLayout;
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL  // VkImageLayout       m_SubpassLayout;
            };
            workspace.upAttachments.push_back(attachment);

            --mipLevel;
        }
    }
}

void BloomPipeline::CreateRenderPasses()
{
    { // down
    // use any image from m_AttachmentsDown since they all have VK_ATTACHMENT_LOAD_OP_CLEAR,
    // e.g., m_attachmentsDown[0] -> mip level 1
        CreateBloomPass(workspaces[0].downAttachments[0], s_RenderPassDown->renderpass);
    }
    { // up
        // use any image from m_AttachmentsUp since they all have VK_ATTACHMENT_LOAD_OP_CLEAR,
        // m_attachmentsUp[0] -> mip level 'NUMBER_OF_MIPMAPS - 2'
        CreateBloomPass(workspaces[0].upAttachments[0], s_RenderPassUp->renderpass);
    }
}

void BloomPipeline::CreateFrameBuffers(Workspace& workspace)
{
    for (int index = 0; index < BLOOM_N_DOWNSAMPLED_IMGS; ++index)
    {
        auto& attachment = workspace.downAttachments[index]; // m_attachmentsDown[0] -> mip level 1
        CreateBloomFrameBuffer(attachment, workspace.downFB[index], s_RenderPassDown->renderpass);
    }

    for (int index = 0; index < BLOOM_N_DOWNSAMPLED_IMGS; ++index)
    {
        auto& attachment = workspace.upAttachments[index]; // m_attachmentsDown[0] -> mip level 1
        CreateBloomFrameBuffer(attachment, workspace.upFB[index], s_RenderPassUp->renderpass);
    }
}

void BloomPipeline::DestroyWorkspaces()
{
    for (Workspace& workspace : workspaces) 
    {
        // Clean up bloom image views
        for (VkImageView& bloomImageView : workspace.m_BloomImageViews) {
            if (bloomImageView != VK_NULL_HANDLE) {
                vkDestroyImageView(VulkanContext::GetDevice(), bloomImageView, nullptr);
                bloomImageView = VK_NULL_HANDLE;
            }
        }

        // Clean up framebuffers
        for (VkFramebuffer& framebuffer : workspace.upFB) {
            if (framebuffer != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(VulkanContext::GetDevice(), framebuffer, nullptr);
                framebuffer = VK_NULL_HANDLE;
            }
        }

        for (VkFramebuffer& framebuffer : workspace.downFB) {
            if (framebuffer != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(VulkanContext::GetDevice(), framebuffer, nullptr);
                framebuffer = VK_NULL_HANDLE;
            }
        }
    }
}

void BloomPipeline::CreateBloomPass(const Attachment& attachment, VkRenderPass& rp)
{
    VkAttachmentDescription attachmentDescription = {};
    attachmentDescription.format = attachment.format;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = attachment.loadOp;
    attachmentDescription.storeOp = attachment.storeOp;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.initialLayout = attachment.initialLayout;
    attachmentDescription.finalLayout = attachment.finalLayout;

    VkAttachmentReference attachmentReference = {};
    attachmentReference.attachment = 0;
    attachmentReference.layout = attachment.subpassLayout;

    VkSubpassDescription subpass = {};
    subpass.flags = 0;
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachmentReference;
    subpass.pResolveAttachments = nullptr;
    subpass.pDepthStencilAttachment = nullptr;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;

    // dependencies
    std::array<VkSubpassDependency, 2> subpassDependencies{};
    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;               // Index of the render pass being depended upon by dstSubpass
    subpassDependencies[0].dstSubpass = 0; // The index of the render pass depending on srcSubpass
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // What pipeline stage must have completed for the dependency
    subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;                // What pipeline stage is waiting on the dependency
    subpassDependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT; // What access scopes influence the dependency
    subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // What access scopes are waiting on the dependency
    subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT; // Other configuration about the dependency

    subpassDependencies[1].srcSubpass = 0;
    subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[1].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &attachmentDescription;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = subpassDependencies.data();

    vkCreateRenderPass(VulkanContext::GetDevice(), &renderPassInfo, nullptr, &rp);
}

void BloomPipeline::CreateBloomFrameBuffer(const Attachment& attachment, VkFramebuffer& fb, VkRenderPass rp)
{
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = rp;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &attachment.imageView;
    framebufferInfo.width = attachment.extent.width;
    framebufferInfo.height = attachment.extent.height;
    framebufferInfo.layers = 1;
    vkCreateFramebuffer(VulkanContext::GetDevice(), &framebufferInfo, nullptr, &fb);
}

void BloomPipeline::CreateBloomPipelines()
{
    std::vector< VkDynamicState > dynamic_states{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    std::array< VkPipelineColorBlendAttachmentState, 1 > attachment_states{
        VkPipelineColorBlendAttachmentState{
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        }
    };

    VkPipelineVertexInputStateCreateInfo vertexInput
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr,
    };

    VulkanGraphicsPipelineBuilder::Start()
        .SetDynamicStates(dynamic_states)
        .SetVertexInput(&vertexInput)
        .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .SetColorBlending((uint32_t)attachment_states.size(), attachment_states.data())
        .SetRasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .Build("../spv/shaders/passthrough.vert.spv", "../spv/shaders/postprocessing/bloom_down_sample.frag.spv", &m_BloomPipelineDown, m_BloomPipelineLayout, s_RenderPassDown->renderpass)
        .Build("../spv/shaders/passthrough.vert.spv", "../spv/shaders/postprocessing/bloom_up_sample.frag.spv", &m_BloomPipelineUp, m_BloomPipelineLayout, s_RenderPassUp->renderpass);
}

void BloomPipeline::CreateBloomPipelineLayout()
{
    std::vector<VkDescriptorSetLayout> layouts{
        set0_MipViewsLayout
    };

    //setup push constants
    VkPushConstantRange push_constant{
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(BloomPush),
    };

    VkPipelineLayoutCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = uint32_t(layouts.size()),
        .pSetLayouts = layouts.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constant,
    };

    VulkanContext::VK(vkCreatePipelineLayout(VulkanContext::GetDevice(), &create_info, nullptr, &m_BloomPipelineLayout));
}

void BloomPipeline::CreateDescriptors(Workspace& workspace)
{
    std::vector<VkDescriptorBindingFlagsEXT> descriptorBindingFlags = {
        VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT,
    };
    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT,
        .bindingCount = 1,
        .pBindingFlags = descriptorBindingFlags.data()
    };

    std::vector<uint32_t> variableDesciptorCounts = { BLOOM_MIP_LEVELS };

    VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variableDescriptorInfoAI =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
        .descriptorSetCount = static_cast<uint32_t>(variableDesciptorCounts.size()),
        .pDescriptorCounts = variableDesciptorCounts.data(),
    };

    // grab all texture information
    std::vector<VkDescriptorImageInfo> hdrMapDescriptors;
    hdrMapDescriptors.reserve(BLOOM_MIP_LEVELS);
    for (int mipLevel = 0; mipLevel < BLOOM_MIP_LEVELS; ++mipLevel)
    {
        VkDescriptorImageInfo descriptorImageInfo{};
        descriptorImageInfo.sampler = workspace.bloomImage->getSampler();
        descriptorImageInfo.imageView = workspace.m_BloomImageViews[mipLevel];
        descriptorImageInfo.imageLayout = workspace.bloomImage->getLayout();

        hdrMapDescriptors.emplace_back(descriptorImageInfo);
    }

    DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
        .BindImage(0, hdrMapDescriptors.data(),
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, BLOOM_MIP_LEVELS)
        .Build(workspace.set0_MipViews, set0_MipViewsLayout, &setLayoutBindingFlags,
#if (defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT))
            // SRS - increase the per-stage descriptor samplers limit on macOS (maxPerStageDescriptorUpdateAfterBindSamplers > maxPerStageDescriptorSamplers)
            VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT
#else
            0 /*VkDescriptorSetLayoutCreateFlags*/
#endif
            , &variableDescriptorInfoAI);
}
