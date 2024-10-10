#include "IBLUtilsApplication.h"

#include "core/resources/Files.hpp"
#include "utils/Logger.hpp"
#include "core/Bitmap.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"

#include "backend/shader/VulkanShader.h"

IBLUtilsApplication::IBLUtilsApplication(IBLUtilsApplicationSpecification& specs_)
{
	specs = specs_;

    ApplicationSpecification appSpecs;
    appSpecs.alternativeApplication = true;
    app = std::make_unique<Application>(appSpecs);
}

IBLUtilsApplication::~IBLUtilsApplication()
{
}

const float pi_over_4 = glm::pi<float>() / 4;
const float pi_over_2 = glm::pi<float>() / 2;

static inline glm::vec2 SampleConcentricDisc(glm::vec2 in)
{
    glm::vec2 offset = in * 2.0f - 1.0f;
    if (offset == glm::vec2(0))
        return glm::vec2(0);

    float theta, r;

    if (std::abs(offset.x) > std::abs(offset.y)) {
        r = offset.x;
        theta = pi_over_4 * (offset.y / offset.x);
    }
    else {
        r = offset.y;
        theta = pi_over_2 - pi_over_4 * (offset.x / offset.y);
    }
    return r * glm::vec2(std::cos(theta), std::sin(theta));
}

static inline glm::vec3 CosineSampleHemisphere(float u1, float u2) 
{
    glm::vec2 d = SampleConcentricDisc(glm::vec2(u1, u2));
    float z = std::sqrt(std::max(0.0f, 1.0f - d.x * d.x - d.y * d.y));
    return glm::vec3(d.x, z, d.y);
}

glm::vec3 CubeUVtoCartesian(int index, glm::vec2 uv)
{
    glm::vec3 xyz(0);
    // convert range 0 to 1 to -1 to 1
    uv = uv * 2.0f - 1.0f;
    switch (index)
    {
    case 0:
        xyz = glm::vec3(1.0f, -uv.y, -uv.x);
        break;  // POSITIVE X
    case 1:
        xyz = glm::vec3(-1.0f, -uv.y, uv.x);
        break;  // NEGATIVE X
    case 2:
        xyz = glm::vec3(uv.x, 1.0f, uv.y);
        break;  // POSITIVE Y
    case 3:
        xyz = glm::vec3(uv.x, -1.0f, -uv.y);
        break;  // NEGATIVE Y
    case 4:
        xyz = glm::vec3(uv.x, -uv.y, 1.0f);
        break;  // POSITIVE Z
    case 5:
        xyz = glm::vec3(uv.x, -uv.y, -1.0f);
        break;  // NEGATIVE Z
    }
    return glm::normalize(xyz);
}

glm::vec2 CartesianToCubeUV(const glm::vec3& dir, uint32_t& faceIndex) {
    glm::vec3 absDir = glm::abs(dir); // Get absolute values for comparison
    glm::vec2 uv(0);

    if (absDir.x >= absDir.y && absDir.x >= absDir.z) {
        if (dir.x > 0) {
            // POSITIVE X (index 0)
            uv = glm::vec2(-dir.z, -dir.y) / absDir.x;
            faceIndex = 0;
        }
        else {
            // NEGATIVE X (index 1)
            uv = glm::vec2(dir.z, -dir.y) / absDir.x;
            faceIndex = 1;
        }
    }
    else if (absDir.y >= absDir.x && absDir.y >= absDir.z) {
        if (dir.y > 0) {
            // POSITIVE Y (index 2)
            uv = glm::vec2(dir.x, dir.z) / absDir.y;
            faceIndex = 2;
        }
        else {
            // NEGATIVE Y (index 3)
            uv = glm::vec2(dir.x, -dir.z) / absDir.y;
            faceIndex = 3;
        }
    }
    else {
        if (dir.z > 0) {
            // POSITIVE Z (index 4)
            uv = glm::vec2(dir.x, -dir.y) / absDir.z;
            faceIndex = 4;
        }
        else {
            // NEGATIVE Z (index 5)
            uv = glm::vec2(-dir.x, -dir.y) / absDir.z;
            faceIndex = 5;
        }
    }

    // Convert UV range from [-1, 1] to [0, 1]
    uv = uv * 0.5f + 0.5f;

    return uv;
}

// Function to sample color from a cubemap given a direction
glm::u8vec4 SampleFromCubemap(const uint8_t* cubemap, glm::vec2 uv, uint32_t dim, int index) {
    uint32_t pixelX = static_cast<uint32_t>(uv.x * ((float)dim - 1));
    uint32_t pixelY = static_cast<uint32_t>(uv.y * ((float)dim - 1));

    // Calculate the byte index for the sampled color
    uint32_t faceOffset = index * dim * dim * 4; // Assuming 4 bytes per pixel (RGB)
    uint32_t pixelIndex = faceOffset + (pixelY * dim + pixelX) * 4; // RGB

    assert(pixelIndex < (dim * dim * 6 * 4));

    // Retrieve color value from the cubemap
    glm::u8vec4 color = {
        cubemap[pixelIndex],
        cubemap[pixelIndex + 1],
        cubemap[pixelIndex + 2],
        cubemap[pixelIndex + 3],
    };

    return color; // Return the sampled color as glm::u8vec4
}

#include "math/Math.hpp"
#include "math/color/Color.hpp"
#include "glm/gtx/string_cast.hpp"

inline glm::vec4 U8Vec4ToVec4(const glm::u8vec4& u8vec) {
    return glm::vec4(
        static_cast<float>(u8vec.r) / 255.0f,
        static_cast<float>(u8vec.g) / 255.0f,
        static_cast<float>(u8vec.b) / 255.0f,
        static_cast<float>(u8vec.a) / 255.0f
    );
}

inline glm::u8vec4 Vec4ToU8Vec4(const glm::vec4& vec) {
    return glm::u8vec4(
        static_cast<unsigned char>(std::clamp(vec.r * 255.0f, 0.0f, 255.0f)),
        static_cast<unsigned char>(std::clamp(vec.g * 255.0f, 0.0f, 255.0f)),
        static_cast<unsigned char>(std::clamp(vec.b * 255.0f, 0.0f, 255.0f)),
        static_cast<unsigned char>(std::clamp(vec.a * 255.0f, 0.0f, 255.0f))
    );
}

void ComputeTangentBitangent(const glm::vec3& normal, glm::vec3& tangent, glm::vec3& bitangent) 
{
    glm::vec3 reference = (fabs(normal.y) < 0.999f) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    tangent = glm::normalize(glm::cross(normal, reference));
    bitangent = glm::normalize(glm::cross(normal, tangent));
}


#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

void printProgress(float percentage) {
    int val = (int)(percentage * 100);
    int lpad = (int)(percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush(stdout);
}

void IBLUtilsApplication::Run()
{
    CreateComputePipeline();
    Prepare();
    ExecuteComputeShader();
    SaveAsImage();

    inputImg.reset();
    storageImg.reset();
    m_DescriptorAllocator.Cleanup();

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

void IBLUtilsApplication::RunCPUBlit()
{
    Bitmap inBitMap = Bitmap(Files::Path(specs.inFile));
    NE_INFO("Loaded input file: {}, size: {}x{}", specs.inFile, inBitMap.size.x, inBitMap.size.y);

    const uint32_t dim = 32; // out dim of the lambertian LUT
    const uint32_t samples = 1024 * 4; // out dim of the lambertian LUT

    Bitmap outBitMap = Bitmap(glm::vec2(dim, dim * 6), 4);
    std::vector<glm::u8vec4> rgbData;

    int progress = 0;

    for (int face = 0; face < 6; ++face)
    {
        for (int row = 0; row < dim; ++row)
        {
            for (int col = 0; col < dim; ++col)
            {
                glm::vec4 irradiance(0);
                glm::vec3 N = CubeUVtoCartesian(face, glm::vec2(col / (float)dim, row / (float)dim)); // sample a direction from output UV to direction

                // tangent space to world
                glm::vec3 tangent, bitangent;
                ComputeTangentBitangent(N, tangent, bitangent);
                //size_t samples = 0;
                //for (int bface = 0; bface < 6; ++bface)
                //{
                //    for (uint32_t brow = 0; brow < inBitMap.size.x; ++brow)
                //    {
                //        for (uint32_t bcol = 0; bcol < inBitMap.size.x; ++bcol)
                //        {
                //            glm::vec3 pixelDirection = CubeUVtoCartesian(bface, glm::vec2(bcol / (float)inBitMap.size.x, brow / (float)inBitMap.size.x));
                //            float cos_theta = glm::dot(N, pixelDirection);
                //            if (cos_theta < 0)
                //                continue;
                //            else
                //            {
                //                // Calculate the byte index for the sampled color
                //                uint32_t faceOffset = bface * inBitMap.size.x * inBitMap.size.x * 4; // Assuming 4 bytes per pixel (RGB)
                //                uint32_t pixelIndex = faceOffset + (brow * inBitMap.size.x + bcol) * 4; // RGB

                //                // Retrieve color value from the cubemap
                //                glm::u8vec4 colorAtPixel = {
                //                    cubemap[pixelIndex],
                //                    cubemap[pixelIndex + 1],
                //                    cubemap[pixelIndex + 2],
                //                    cubemap[pixelIndex + 3],
                //                };

                //                glm::vec4 rgba = rgbe_to_float(colorAtPixel);
                //                float pdf = cos_theta * glm::one_over_pi<float>(); // cos theta / pi
                //                irradiance += rgba / pdf;
                //                samples++;
                //            }
                //        }
                //    }
                //}

                for (int xx = 0; xx < samples; xx++)
                {
                    // spherical to cartesian (in tangent space)
                    glm::vec3 sampleDir = CosineSampleHemisphere(Math::Random(), Math::Random()); // sample a direction from cosine given a normal
                    assert(sampleDir.y >= 0);

                    // sample direction in world space
                    glm::vec3 wsSampleDir = sampleDir.x * tangent + sampleDir.y * bitangent + sampleDir.z * N;

                    // sample from environment
                    uint32_t uvFace;
                    glm::vec2 wsUV = CartesianToCubeUV(glm::normalize(wsSampleDir), uvFace);
                    assert(wsUV.x >= 0 && wsUV.y >= 0 && wsUV.x <= 1 && wsUV.y <= 1);
                    glm::u8vec4 sampledColor = SampleFromCubemap(inBitMap.data.get(), wsUV, inBitMap.size.x, uvFace); // sample from direction from the input UV
                    glm::vec4 rgb = rgbe_to_float(sampledColor);

                    // cosine theta = dot(y, (0,1,0)) = y
                    float pdf = sampleDir.y * glm::one_over_pi<float>(); // cos theta / pi
                    auto newIrradiance = rgb / pdf;
                    irradiance += newIrradiance; // convert to u8vec4 for averaging
                }

                irradiance /= (float)samples;
                rgbData.push_back(float_to_rgbe(irradiance));

                progress++;
                printProgress((float)progress / (dim * dim * 6));
            }
        }
    }

    assert(rgbData.size() == 6 * dim * dim);

    assert(outBitMap.GetLength() == rgbData.size() * 4);
    memcpy(outBitMap.data.get(), rgbData.data(), rgbData.size() * 4);

    outBitMap.Write(Files::Path(specs.outFile, false));
}

void IBLUtilsApplication::CreateComputePipeline()
{
    const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    // Check if requested image format supports image storage operations required for storing pixel from the compute shader
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(*VulkanContext::Get()->getPhysicalDevice(), format, &formatProperties);
    assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);

    // make the input image
    inputImg = std::make_shared<ImageCube>(Files::Path(specs.inFile), format,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, false);
    VkDescriptorImageInfo inputTex
    {
        .sampler = inputImg->getSampler(),
        .imageView = inputImg->getView(),
        .imageLayout = inputImg->getLayout()
    };

    storageImg = std::make_shared<ImageCube>(glm::vec2(specs.outdim, specs.outdim), 
        format, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, false);
    VkDescriptorImageInfo storageTex
    {
        .sampler = storageImg->getSampler(),
        .imageView = storageImg->getView(),
        .imageLayout = storageImg->getLayout()
    };

    DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
        .BindImage(0, &inputTex, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
        .BindImage(1, &storageTex, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
        .Build(set1_Texture, set1_TextureLayout);
}

void IBLUtilsApplication::Prepare()
{
    // create pipeline layout
    std::array< VkDescriptorSetLayout, 1 > layouts{
        set1_TextureLayout,
    };

    VkPipelineLayoutCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = uint32_t(layouts.size()),
        .pSetLayouts = layouts.data(),
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };

    VulkanContext::VK_CHECK(vkCreatePipelineLayout(VulkanContext::GetDevice(), &create_info, nullptr, &m_PipelineLayout));

    VulkanShader vertModule("../spv/shaders/compute/lambertian.comp.spv", VulkanShader::ShaderStage::Compute);

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

void IBLUtilsApplication::ExecuteComputeShader()
{
    CommandBuffer cmd(true, VK_QUEUE_COMPUTE_BIT);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, &set1_Texture, 0, 0);
    vkCmdDispatch(cmd, storageImg->getExtent().width / 4, storageImg->getExtent().height / 4, 6);
    cmd.Submit(nullptr, nullptr, fence);

    vkWaitForFences(VulkanContext::GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
}

void IBLUtilsApplication::SaveAsImage()
{
    std::vector<uint8_t> allBytes;
    for (int i = 0; i < 6; i++)
    {
        auto bitmap = storageImg->getBitmap(0, i, 4);

        size_t currOffset = allBytes.size();
        allBytes.resize(allBytes.size() + bitmap->GetLength());
        memcpy(allBytes.data() + currOffset, bitmap->data.get(), bitmap->GetLength());
    }
    glm::uvec2 size(specs.outdim, specs.outdim * 6);
    Bitmap::Write(Files::Path(specs.outFile, false), allBytes.data(), size, 4);
}
