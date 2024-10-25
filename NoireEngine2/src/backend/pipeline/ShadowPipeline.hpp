#pragma once

#include "VulkanPipeline.hpp"
#include "math/Math.hpp"
#include "backend/images/ImageDepth.hpp"
#include "backend/buffers/Buffer.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"
#include <array>

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

	// Contains all resources required for a single shadow map cascade
	struct Cascade
	{
		VkFramebuffer frameBuffer;
		std::unique_ptr<ImageDepth> depthAttachment;
		glm::mat4 viewProjMatrix;
		float splitDepth;
	};

	// a cascade pass has multiple cascades
	struct CascadePass
	{
		int32_t width, height;
		std::array<Cascade, SHADOW_MAP_CASCADE_COUNT> cascades;
	};

	//struct UBOFS {
	//	float cascadeSplits[4];
	//	glm::mat4 inverseViewMat;
	//	glm::vec3 lightDir;
	//	float _pad;
	//	int32_t colorCascades;
	//} uboFS;

	// naive shadowmap pass, used for spotlights
	struct ShadowMapPass 
	{
		int32_t width, height;
		VkFramebuffer frameBuffer;
		std::unique_ptr<ImageDepth> depthAttachment;
	};
	const std::vector<ShadowMapPass>& getShadowPasses() const { return m_ShadowMapPasses; }

private:
	// Global pipeline variables ////////////////////////////////////////////
	ObjectPipeline*		p_ObjectPipeline = nullptr;

	struct Workspace
	{
		Buffer LightSpaces_Src;
		Buffer LightSpaces;

		VkDescriptorSet set0_Lightspaces;
	};
	std::vector<Workspace> workspaces;

	VkDescriptorSetLayout set0_LightspacesLayout;
	DescriptorAllocator m_Allocator;

	// Shadow Mapping Naive ////////////////////////////////////////////

	VkPipelineLayout	m_ShadowMapPassPipelineLayout;
	VkRenderPass		m_ShadowMapRenderPass;
	VkPipeline			m_ShadowMapPipeline;

	struct Push 
	{
		int lightspaceID;
	};

	std::vector<ShadowMapPass> m_ShadowMapPasses;

	void ShadowMap_CreateRenderPasses();
	void ShadowMap_CreateDescriptors();
	void ShadowMap_CreatePipelineLayout();
	void ShadowMap_CreateGraphicsPipeline();
	void ShadowMap_BeginRenderPass(const CommandBuffer& cmdBuffer, uint32_t index);

	// Shadow cascading ////////////////////////////////////////////

	VkRenderPass		m_CascadeRenderPass;
	VkPipelineLayout	m_CascadePassPipelineLayout;
	VkPipeline			m_CascadePipeline;

	int32_t displayDepthMapCascadeIndex = 0;
	float cascadeSplitLambda = 0.95f;
	
	struct CascadePush 
	{
		glm::vec4 position;
		uint32_t cascadeIndex;
	};

	std::vector<CascadePass> m_ShadowCascadePasses;

	void Cascade_CreateRenderPasses();
	void Cascade_CreateDescriptors();
	void Cascade_CreatePipelineLayout();
	void Cascade_CreateGraphicsPipeline();
	void Cascade_BeginRenderPass(const CommandBuffer& cmdBuffer, uint32_t index);
};

