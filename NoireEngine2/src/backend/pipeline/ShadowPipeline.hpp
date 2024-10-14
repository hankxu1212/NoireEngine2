#pragma once

#include "VulkanPipeline.hpp"
#include "math/Math.hpp"
#include "backend/images/ImageDepth.hpp"
#include "backend/buffers/Buffer.hpp"

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

private:
	VkPipeline			m_PipelineOffscreen;
	VkPipeline			m_PipelineShadow;
	VkPipelineLayout	m_PipelineLayout;

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

	struct UniformDataScene {
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
		glm::mat4 depthBiasMVP;
		glm::vec4 lightPos;
		// Used for depth map visualization
		float zNear;
		float zFar;
	} uniformDataScene;

	struct UniformDataOffscreen {
		glm::mat4 depthMVP;
	} uniformDataOffscreen;

	Buffer m_BufferScene;
	Buffer m_BufferOffscreen;

	VkDescriptorSet set0_Offscreen;
	VkDescriptorSet set0_OffscreenLayout;

	struct OffscreenPass {
		int32_t width, height;
		VkFramebuffer frameBuffer;
		VkRenderPass renderPass;
		VkDescriptorImageInfo descriptor;
	} offscreenPass{};

	std::unique_ptr<ImageDepth> m_DepthAttachment;

	const VkFormat offscreenDepthFormat{ VK_FORMAT_D16_UNORM };

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
	void CreateDescriptors();

	void UpdateLight();
};

