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

	void CreateRenderPass() override;

	void Rebuild() override;

	void CreatePipeline() override;

	void Prepare(const Scene* scene, const CommandBuffer& commandBuffer);

	void Render(const Scene* scene, const CommandBuffer& commandBuffer) override;

	ImageDepth* GetShadowImage() { return m_DepthAttachment.get(); }

	// Keep depth range as small as possible
	// for better shadow map precision
	float zNear = 1.0f;
	float zFar = 96.0f;

	glm::vec3 lightPos = glm::vec3();

	// Depth bias (and slope) are used to avoid shadowing artifacts
	// Constant depth bias factor (always applied)
	float depthBiasConstant = 1.25f;
	// Slope depth bias factor, applied depending on polygon's slope
	float depthBiasSlope = 1.75f;

	float lightFOV = 45.0f;

private:
	VkPipeline			m_Pipeline;
	VkPipelineLayout	m_PipelineLayout;


	struct UniformDataOffscreen 
	{
		glm::mat4 depthMVP;
	} m_OffscreenUniform;

	Buffer m_BufferOffscreenUniform;

	VkDescriptorSet set0_Offscreen;
	VkDescriptorSetLayout set0_OffscreenLayout;

	DescriptorAllocator m_Allocator;

	struct OffscreenPass {
		int32_t width, height;
		VkFramebuffer frameBuffer;
		VkRenderPass renderPass;
	} offscreenPass{};

	std::unique_ptr<ImageDepth> m_DepthAttachment;

	// Shadow map dimension
#if defined(__ANDROID__)
	// Use a smaller size on Android for performance reasons
	const uint32_t shadowMapize{ 1024 };
#else
	const uint32_t shadowMapize{ 2048 };
#endif

	void PrepareOffscreenFrameBuffer();
	void PrepareUniformBuffers();

	void UpdateUniforms();
	void UpdateLight();

	void CreateDescriptors();
	void CreatePipelineLayout();
	void CreateGraphicsPipeline();

	void BindDescriptors(const CommandBuffer& cmdBuffer);

	void BeginRenderPass(const CommandBuffer& cmdBuffer);
};

