#include "VulkanGraphicsPipelineBuilder.hpp"
#include "backend/shader/VulkanShader.h"

VulkanGraphicsPipelineBuilder VulkanGraphicsPipelineBuilder::Start()
{
    VulkanGraphicsPipelineBuilder builder;

    builder.SetViewport().SetMultisample().SetRasterization().SetDepthStencil();

    return builder;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetDynamicStates(const std::vector<VkDynamicState>& dynamicStates)
{
    dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = uint32_t(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetInputAssembly(VkPrimitiveTopology topology)
{
    inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = topology,
        .primitiveRestartEnable = VK_FALSE
    };

    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetVertexInput(const VkPipelineVertexInputStateCreateInfo* input)
{
    vertexInput = input;
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetViewport(uint32_t viewportCnt, uint32_t scissorCnt)
{
    viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = viewportCnt,
        .scissorCount = scissorCnt,
    };

    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetRasterization(VkPolygonMode mode, VkCullModeFlags cull, VkFrontFace front, float lineW)
{
    rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = mode,
        .cullMode = cull,
        .frontFace = front,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = lineW,
    };

    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetMultisample(VkSampleCountFlagBits samples)
{
    multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = samples,
        .sampleShadingEnable = VK_FALSE,
    };

    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetDepthStencil(VkBool32 depthTest, VkBool32 depthWrite, VkBool32 depthBounds, VkBool32 stencilTest, VkCompareOp op)
{
    depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = depthTest,
        .depthWriteEnable = depthWrite,
        .depthCompareOp = op,
        .depthBoundsTestEnable = depthBounds,
        .stencilTestEnable = stencilTest,
    };

    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetColorBlending(uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState* attachments)
{
    colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = attachmentCount,
        .pAttachments = attachments,
        .blendConstants{0.0f, 0.0f, 0.0f, 0.0f},
    };
    return *this;
}

void VulkanGraphicsPipelineBuilder::Build(const std::string& vert, const std::string& frag, VkPipeline* pipeline, VkPipelineLayout layout, VkRenderPass renderpass, uint32_t subpass)
{
    VulkanShader vertModule(vert, VulkanShader::ShaderStage::Vertex);
    VulkanShader fragModule(frag, VulkanShader::ShaderStage::Frag);

    std::array< VkPipelineShaderStageCreateInfo, 2 > stages{
        vertModule.shaderStage(),
        fragModule.shaderStage()
    };

    VkGraphicsPipelineCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = uint32_t(stages.size()),
        .pStages = stages.data(),
        .pVertexInputState = vertexInput,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = layout,
        .renderPass = renderpass,
        .subpass = subpass,
    };

    VulkanContext::VK(
        vkCreateGraphicsPipelines(VulkanContext::GetDevice(), VulkanContext::Get()->getPipelineCache(), 1, &create_info, nullptr, pipeline),
        "[Vulkan] Create pipeline failed");
}
