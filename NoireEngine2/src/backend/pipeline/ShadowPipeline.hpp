#pragma once

#include "VulkanPipeline.hpp"
#include "math/Math.hpp"
#include "backend/images/ImageDepth.hpp"
#include "backend/buffers/Buffer.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"
#include <array>

class ObjectPipeline;

#define SHADOW_MAP_CASCADE_COUNT 4
#define OMNI_SHADOWMAPS_COUNT 6

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

	// naive shadowmap pass, used for spotlights
	struct ShadowMapPass 
	{
		uint32_t width, height;
		VkFramebuffer frameBuffer;
		std::unique_ptr<ImageDepth> depthAttachment;
	};
	const std::vector<ShadowMapPass>& getShadowPasses() const { return m_ShadowMapPasses; }

	// Contains all resources required for a single shadow map cascade
	struct Cascade
	{
		VkFramebuffer frameBuffer;
		std::unique_ptr<ImageDepth> depthAttachment;
	};

	// a cascade pass has multiple cascades
	struct CascadePass
	{
		uint32_t width, height;
		std::array<Cascade, SHADOW_MAP_CASCADE_COUNT> cascades;
	};
	const std::vector<CascadePass>& getCascadePasses() const { return m_CascadePasses; }
	
	// omni-directional shadowmap passes for point lights
	struct OmniCubeface
	{
		VkFramebuffer frameBuffer;
		std::unique_ptr<ImageDepth> depthAttachment;
	};

	// a cascade pass has multiple cascades
	struct OmniPass
	{
		uint32_t width, height;
		std::array<OmniCubeface, OMNI_SHADOWMAPS_COUNT> cubefaces;
	};
	const std::vector<OmniPass>& getOmniPasses() const { return m_OmniPasses; }

	// to be used in gizmos UI
	static inline uint32_t PCFSamples = 32;
	static inline uint32_t PCSSOccluderSamples = 32;

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

	void ShadowMap_CreateRenderPasses(uint32_t numPasses);
	void ShadowMap_CreateDescriptors();
	void ShadowMap_CreatePipelineLayout();
	void ShadowMap_CreateGraphicsPipeline();
	void ShadowMap_BeginRenderPass(const CommandBuffer& commandBuffer, uint32_t passIndex);

	// Shadow cascading ////////////////////////////////////////////
	
	// cascades will push SHADOW_MAP_CASCADE_COUNT lightspace matrices into the lightspace buffer, instead of 1
	std::vector<CascadePass> m_CascadePasses;

	void Cascade_CreateRenderPasses(uint32_t numPasses);
	void Cascade_SetViewports(const CommandBuffer& commandBuffer, uint32_t passIndex);
	void Cascade_BeginRenderPass(const CommandBuffer& commandBuffer, uint32_t passIndex, uint32_t cascadeIndex);

	// Omni-directional point light shadowmapping ////////////////////////////////////////////
	// pushes 6 faces in a single pass
	std::vector<OmniPass> m_OmniPasses;

	void Omni_CreateRenderPasses(uint32_t numPasses);
	void Omni_SetViewports(const CommandBuffer& commandBuffer, uint32_t passIndex);
	void Omni_BeginRenderPass(const CommandBuffer& commandBuffer, uint32_t passIndex, uint32_t faceIndex);
};

