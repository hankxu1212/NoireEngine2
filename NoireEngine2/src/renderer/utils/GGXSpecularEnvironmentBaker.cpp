#include "GGXSpecularEnvironmentBaker.hpp"
#include "IBLUtilsApplication.hpp"

#include "core/resources/Files.hpp"
#include "utils/Logger.hpp"
#include "core/Bitmap.hpp"
#include "backend/shader/VulkanShader.h"
#include "math/Math.hpp"
#include "math/color/Color.hpp"

#include "glm/gtc/constants.hpp"
#include <format>

GGXSpecularEnvironmentBaker::GGXSpecularEnvironmentBaker(IBLUtilsApplicationSpecification* specs_) :
    specs(specs_)
{
}

void GGXSpecularEnvironmentBaker::Run()
{
    Setup();

    glm::uvec2 dim = { inputImg->getExtent().width / 2, inputImg->getExtent().width / 2 };

    // MIP_LEVEL = 0 is original image
    for (int i = 1; i < GGX_MIP_LEVELS; ++i) {
        miplevel = i;

        CreateComputePipeline(dim);
        Prepare();
        ExecuteComputeShader();
        SaveAsImage(dim);
        Cleanup();

        dim /= 2;
    }
    inputImg.reset();
    m_DescriptorAllocator.Cleanup();
}

void GGXSpecularEnvironmentBaker::Setup()
{
    const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    // Check if requested image format supports image storage operations required for storing pixel from the compute shader
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(*VulkanContext::Get()->getPhysicalDevice(), format, &formatProperties);
    assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);

    // make the input image
    inputImg = std::make_shared<ImageCube>(Files::Path(specs->inFile), format,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, false);
}

void GGXSpecularEnvironmentBaker::Cleanup()
{
    storageImg.reset();

    if (m_PipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(VulkanContext::GetDevice(), m_PipelineLayout, nullptr);
        m_PipelineLayout = VK_NULL_HANDLE;
    }

    if (m_Pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(VulkanContext::GetDevice(), m_Pipeline, nullptr);
        m_Pipeline = VK_NULL_HANDLE;
    }

    vkDestroyFence(VulkanContext::GetDevice(), fence, nullptr);
}

void GGXSpecularEnvironmentBaker::CreateComputePipeline(glm::uvec2 dim)
{
    const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    storageImg = std::make_shared<ImageCube>(dim,
        format, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, false);

    VkDescriptorImageInfo inputTex = inputImg->GetDescriptorInfo();
    VkDescriptorImageInfo storageTex = storageImg->GetDescriptorInfo();

    DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
        .BindImage(0, &inputTex, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
        .BindImage(1, &storageTex, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
        .Build(set0_Textures, set0_TextureLayout);
}

void GGXSpecularEnvironmentBaker::Prepare()
{
    std::array< VkDescriptorSetLayout, 1 > layouts{
        set0_TextureLayout,
    };

    VkPipelineLayoutCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = uint32_t(layouts.size()),
        .pSetLayouts = layouts.data(),
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };

    VulkanContext::VK_CHECK(vkCreatePipelineLayout(VulkanContext::GetDevice(), &create_info, nullptr, &m_PipelineLayout));

    std::string shaderName = std::format("../spv/shaders/compute/ggx/ggx_hdr_{}.comp.spv", miplevel);
    NE_INFO("Executing compute shader:{}", shaderName);
    VulkanShader vertModule(shaderName, VulkanShader::ShaderStage::Compute);

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.stage = vertModule.shaderStage();

    if (vkCreateComputePipelines(VulkanContext::GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline!");
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(VulkanContext::GetDevice(), &fenceInfo, nullptr, &fence);
}

void GGXSpecularEnvironmentBaker::ExecuteComputeShader()
{
    CommandBuffer cmd(true, VK_QUEUE_COMPUTE_BIT);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, &set0_Textures, 0, 0);

    // dispatch compute
    vkCmdDispatch(cmd, storageImg->getExtent().width / 4, storageImg->getExtent().height / 4, 6);

    cmd.Submit(nullptr, nullptr, fence);

    vkWaitForFences(VulkanContext::GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
}

void GGXSpecularEnvironmentBaker::SaveAsImage(glm::uvec2 dim)
{
    std::string out = specs->outFile.substr(0, specs->outFile.length() - 4) + ".ggx-" + std::to_string(miplevel) + ".png";
    storageImg->SaveAsPNG(out, { dim.x, dim.y * 6 });
    NE_INFO("Written to:{}", Files::Path(out, false));
}