#pragma once

#include "VulkanPipeline.hpp"
#include "math/Math.hpp"
#include "backend/images/ImageDepth.hpp"
#include "backend/buffers/Buffer.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"

class ObjectPipeline;

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
	ObjectPipeline* p_ObjectPipeline = nullptr;

	struct Push 
	{
		int lightspaceID;
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

