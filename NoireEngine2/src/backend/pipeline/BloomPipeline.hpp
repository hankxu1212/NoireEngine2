#pragma once

#include "VulkanPipeline.hpp"
#include "math/Math.hpp"
#include "utils/ThreadPool.hpp"
#include <array>

#include "backend/images/ImageDepth.hpp"
#include "backend/buffers/Buffer.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"
#include "backend/renderpass/Renderpass.hpp"
#include "backend/images/Image2D.hpp"

#define BLOOM_MIP_LEVELS 5
#define BLOOM_N_DOWNSAMPLED_IMGS BLOOM_MIP_LEVELS - 1

class BloomPipeline : public VulkanPipeline
{
public:
    BloomPipeline();

	~BloomPipeline();

	void Rebuild(const std::vector<Image2D*>& workspaces);

	void CreatePipeline() override;

	void Render(const Scene* scene, const CommandBuffer& commandBuffer) override;

    void OnUIRender();

private:
    friend class Renderer;

    struct Attachment
    {
        VkImageView imageView;
        VkFormat format;
        VkExtent2D extent;
        VkAttachmentLoadOp loadOp;
        VkAttachmentStoreOp storeOp;
        VkImageLayout initialLayout;
        VkImageLayout finalLayout;
        VkImageLayout subpassLayout;
    };

    struct Workspace
    {
        Image2D* bloomImage; // just a handle, main thing is in renderer

        std::array<VkImageView, BLOOM_MIP_LEVELS> m_BloomImageViews;
        std::vector<Attachment> upAttachments;
        std::vector<Attachment> downAttachments;
        std::array<VkFramebuffer, BLOOM_N_DOWNSAMPLED_IMGS> upFB, downFB;

        VkDescriptorSet set0_MipViews;
    };

    std::vector<Workspace> workspaces;

    std::unique_ptr<Renderpass> s_RenderPassUp, s_RenderPassDown;

    VkPipeline m_BloomPipelineUp, m_BloomPipelineDown;
    VkPipelineLayout m_BloomPipelineLayout;

    VkDescriptorSetLayout set0_MipViewsLayout;
    DescriptorAllocator m_DescriptorAllocator;

    void CreateImageViews(Workspace& workspace);
    void CreateAttachments(Workspace& workspace);
    void CreateRenderPasses();
    void CreateFrameBuffers(Workspace& workspace);
    void DestroyWorkspaces();

    void CreateBloomPass(const Attachment& attachment, VkRenderPass& rp);
    void CreateBloomFrameBuffer(const Attachment& attachment, VkFramebuffer& fb, VkRenderPass rp);

    void CreateBloomPipelines();
    void CreateBloomPipelineLayout();

    void CreateDescriptors(Workspace& workspace);
    void UpdateDescriptors(Workspace& workspace);
    bool descriptorsCreated = false;

    struct BloomPush
    {
        glm::vec2 texelSize;
        float filterRadius = 0.01f;
        uint32_t mipLevel;
    }m_BloomPush;
};

