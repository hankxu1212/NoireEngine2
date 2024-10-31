#include "EnvironmentBRDFBaker.hpp"
#include "core/resources/Files.hpp"
#include "utils/Logger.hpp"
#include "core/Bitmap.hpp"
#include "IBLUtilsApplication.hpp"
#include "backend/shader/VulkanShader.h"

EnvironmentBRDFBaker::EnvironmentBRDFBaker(IBLUtilsApplicationSpecification* specs_) :
	specs(specs_)
{
}

EnvironmentBRDFBaker::~EnvironmentBRDFBaker()
{
    brdfImage->Destroy();

    if (m_PipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(VulkanContext::GetDevice(), m_PipelineLayout, nullptr);
        m_PipelineLayout = VK_NULL_HANDLE;
    }

    if (m_Pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(VulkanContext::GetDevice(), m_Pipeline, nullptr);
        m_Pipeline = VK_NULL_HANDLE;
    }

    vkDestroyFence(VulkanContext::GetDevice(), fence, nullptr);
    m_DescriptorAllocator.Cleanup();
}

void EnvironmentBRDFBaker::Run()
{
    Setup();
    ExecuteComputeShader();
    SaveAsImage();
}

void EnvironmentBRDFBaker::Setup()
{
    // rg16f
    const VkFormat format = VK_FORMAT_R16G16_UNORM;

    // Check if requested image format supports image storage operations required for storing pixel from the compute shader
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(*VulkanContext::Get()->getPhysicalDevice(), format, &formatProperties);
    assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);

    // make the input image
    brdfImage = std::make_shared<Image2D>(specs->outdim, specs->outdim, format,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT);

    // descriptor
    VkDescriptorImageInfo storageTex = brdfImage->GetDescriptorInfo();
    DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
        .BindImage(0, &storageTex, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
        .Build(set0_Textures, set0_TextureLayout);

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

    std::string shaderName = "../spv/shaders/compute/ggx/ggx_brdf.comp.spv";
    NE_INFO("Executing compute shader:{}", shaderName);
    VulkanShader vertModule(shaderName, VulkanShader::ShaderStage::Compute);

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.stage = vertModule.shaderStage();

    VulkanContext::VK_CHECK(vkCreateComputePipelines(VulkanContext::GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline));

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(VulkanContext::GetDevice(), &fenceInfo, nullptr, &fence);
}

void EnvironmentBRDFBaker::ExecuteComputeShader()
{
    CommandBuffer cmd(true, VK_QUEUE_COMPUTE_BIT);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, &set0_Textures, 0, 0);

    // dispatch compute
    vkCmdDispatch(cmd, brdfImage->getExtent().width / 4, brdfImage->getExtent().height / 4, 1);

    cmd.Submit(nullptr, nullptr, fence);

    vkWaitForFences(VulkanContext::GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
}

void EnvironmentBRDFBaker::SaveAsImage()
{
    std::string out = specs->outFile.substr(0, specs->outFile.length() - 4) + ".brdf.png";
    brdfImage->getBitmap(0, 0, 4)->Write(Files::Path(out, false));
}
