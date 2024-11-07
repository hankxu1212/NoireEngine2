#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <cstdint>

struct VulkanGraphicsPipelineBuilder
{
    static VulkanGraphicsPipelineBuilder Start();
    VulkanGraphicsPipelineBuilder& SetDynamicStates(const std::vector<VkDynamicState>& dynamicStates);
    VulkanGraphicsPipelineBuilder& SetInputAssembly(VkPrimitiveTopology topology);
    VulkanGraphicsPipelineBuilder& SetVertexInput(const VkPipelineVertexInputStateCreateInfo* vertexInput);
    VulkanGraphicsPipelineBuilder& SetViewport(uint32_t viewportCnt = 1, uint32_t scissorCnt = 1);
    VulkanGraphicsPipelineBuilder& SetRasterization(VkPolygonMode mode=VK_POLYGON_MODE_FILL, VkCullModeFlags cull= VK_CULL_MODE_BACK_BIT, VkFrontFace front= VK_FRONT_FACE_COUNTER_CLOCKWISE, float lineW=1);
    VulkanGraphicsPipelineBuilder& SetMultisample(VkSampleCountFlagBits samples=VK_SAMPLE_COUNT_1_BIT);
    VulkanGraphicsPipelineBuilder& SetDepthStencil(VkBool32 depthTest=VK_TRUE, VkBool32 depthWrite=VK_TRUE, VkBool32 depthBounds=VK_FALSE, VkBool32 stencilTest=VK_FALSE, VkCompareOp op=VK_COMPARE_OP_LESS);
    VulkanGraphicsPipelineBuilder& SetColorBlending(uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState* attachments);
    
    VulkanGraphicsPipelineBuilder& Build(const std::string& vert, const std::string& frag, VkPipeline* pipeline, VkPipelineLayout layout, VkRenderPass renderpass, uint32_t subpass = 0);

    // Add members for pipeline creation information
    VkPipelineDynamicStateCreateInfo dynamicState{};
    const VkPipelineVertexInputStateCreateInfo* vertexInput = VK_NULL_HANDLE;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    VkPipelineViewportStateCreateInfo viewportState{};
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    VkPipelineMultisampleStateCreateInfo multisampling{};
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    VkPipelineColorBlendStateCreateInfo colorBlending{};
};

