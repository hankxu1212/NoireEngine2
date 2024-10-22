#pragma once

#include "VulkanPipeline.hpp"
#include "math/Math.hpp"
#include "backend/images/ImageDepth.hpp"
#include "backend/buffers/Buffer.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"

class ShadowPipeline : public VulkanPipeline
{
public:
	ShadowPipeline();
	~ShadowPipeline();

	// creates as many render passes as there are shadow casters in the scene
	void CreateRenderPass() override;

	void Rebuild() override;

	void CreatePipeline() override;

	void Prepare(const Scene* scene, const CommandBuffer& commandBuffer);

	// run a series of render passes on each shadow caster
	void Render(const Scene* scene, const CommandBuffer& commandBuffer) override;

	struct OffscreenPass {
		int32_t width, height;
		VkFramebuffer frameBuffer;
		VkRenderPass renderPass;
		std::unique_ptr<ImageDepth> depthAttachment;
		VkPipeline pipeline;
	};
	const std::vector<OffscreenPass>& getShadowPasses() const { return offscreenpasses; }

private:
	VkPipelineLayout	m_PipelineLayout;

	struct UniformDataOffscreen 
	{
		glm::mat4 depthMVP;
	} m_OffscreenUniform;

	Buffer m_BufferOffscreenUniform;

	VkDescriptorSet set0_Offscreen;
	VkDescriptorSetLayout set0_OffscreenLayout;

	DescriptorAllocator m_Allocator;


	std::vector<OffscreenPass> offscreenpasses;

	// Depth bias (and slope) are used to avoid shadowing artifacts
	// Constant depth bias factor (always applied)
	float depthBiasConstant = 1.25f;
	// Slope depth bias factor, applied depending on polygon's slope
	float depthBiasSlope = 1.75f;

	// Shadow map dimension
#if defined(__ANDROID__)
	// Use a smaller size on Android for performance reasons
	const uint32_t shadowMapize{ 1024 };
#else
	const uint32_t shadowMapize{ 2048 };
#endif

	void CreateDescriptors();
	void CreatePipelineLayout();
	void CreateGraphicsPipeline();

	void UpdateAndBindDescriptors(const CommandBuffer& cmdBuffer, uint32_t index);

	void BeginRenderPass(const CommandBuffer& cmdBuffer, uint32_t index);
};

