#pragma once

#include "VulkanPipeline.hpp"
#include "math/Math.hpp"
#include "backend/images/ImageDepth.hpp"
#include "backend/buffers/Buffer.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"

class ObjectPipeline;

#define SHADOW_MAP_CASCADE_COUNT 4

class ShadowPipeline : public VulkanPipeline
{
public:
	ShadowPipeline(ObjectPipeline* objectPipeline);
	~ShadowPipeline();

	// creates as many render passes as there are shadow casters in the scene
	void CreateRenderPass() override;

	void Rebuild() override;

	void CreatePipeline() override;

	void Prepare(const Scene* scene, const CommandBuffer& commandBuffer);

	// run a series of render passes on each shadow caster
	void Render(const Scene* scene, const CommandBuffer& commandBuffer) override;

	// Resources of the depth map generation pass
	struct DepthPass {
		VkRenderPass renderPass;
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;
	} depthPass;

	struct OffscreenPass {
		int32_t width, height;
		VkFramebuffer frameBuffer;
		VkRenderPass renderPass;
		std::unique_ptr<ImageDepth> depthAttachment;
		VkPipeline pipeline;
	};
	const std::vector<OffscreenPass>& getShadowPasses() const { return offscreenpasses; }

private:
	int32_t displayDepthMapCascadeIndex = 0;
	float cascadeSplitLambda = 0.95f;

	VkPipelineLayout	m_PipelineLayout;
	ObjectPipeline* p_ObjectPipeline = nullptr;

	struct Push 
	{
		int lightspaceID;
	};

	// For simplicity all pipelines use the same push constant block layout
	struct CascadePush {
		glm::vec4 position;
		uint32_t cascadeIndex;
	};

	struct Workspace
	{
		Buffer LightSpaces_Src;
		Buffer LightSpaces;

		VkDescriptorSet set0_Offscreen;
	};
	std::vector<Workspace> workspaces;

	VkDescriptorSetLayout set0_OffscreenLayout;

	DescriptorAllocator m_Allocator;

	std::vector<OffscreenPass> offscreenpasses;

	void CreateRenderPasses();
	void CreateDescriptors();
	void CreatePipelineLayout();
	void CreateGraphicsPipeline();

	void BeginRenderPass(const CommandBuffer& cmdBuffer, uint32_t index);
};

