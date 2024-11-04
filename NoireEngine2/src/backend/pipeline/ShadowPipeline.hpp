#pragma once

#include "VulkanPipeline.hpp"
#include "math/Math.hpp"
#include "utils/ThreadPool.hpp"
#include <array>

#include "backend/images/ImageDepth.hpp"
#include "backend/buffers/Buffer.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"

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
	static inline uint32_t PCSSOccluderSamples = 8;

private:
	// Global pipeline variables ////////////////////////////////////////////
	ObjectPipeline*		p_ObjectPipeline = nullptr;

	struct Workspace
	{
		Buffer LightSpaces_Src;
		Buffer LightSpaces;

		VkDescriptorSet set0_Lightspaces;

		// Multi-threading ////////////////////////////////////////////
		std::unique_ptr<ThreadPool> threadPool;
		std::vector<std::unique_ptr<CommandBuffer>> secondaryCommandBuffers;
	};

	std::vector<Workspace> workspaces;

	VkDescriptorSetLayout set0_LightspacesLayout;
	DescriptorAllocator m_Allocator;

	void CreateDescriptors();
	void CreatePipelineLayout();
	void CreateGraphicsPipeline();
	void SetViewports(const CommandBuffer& commandBuffer, uint32_t width, uint32_t height);
	void BeginRenderPass(const CommandBuffer& commandBuffer, VkFramebuffer frameBuffer, uint32_t width, uint32_t height);

	VkPipelineLayout	m_ShadowMapPassPipelineLayout = VK_NULL_HANDLE;
	VkRenderPass		m_ShadowMapRenderPass = VK_NULL_HANDLE;
	VkPipeline			m_ShadowMapPipeline = VK_NULL_HANDLE;

	struct Push 
	{
		int lightspaceID;
	};

	// Multi-threading ////////////////////////////////////////////
	void PrepareShadowRenderThreads();
	void T_RenderShadows(uint32_t tid, VkCommandBufferInheritanceInfo inheritanceInfo, const Scene* scene, uint32_t lightType, uint32_t width, uint32_t height);

	// Shadow Mapping Naive ////////////////////////////////////////////
	std::vector<ShadowMapPass> m_ShadowMapPasses;
	void ShadowMap_CreateRenderPasses(uint32_t numPasses);

	// Shadow cascading ////////////////////////////////////////////
	// cascades will push SHADOW_MAP_CASCADE_COUNT lightspace matrices into the lightspace buffer, instead of 1
	std::vector<CascadePass> m_CascadePasses;
	void Cascade_CreateRenderPasses(uint32_t numPasses);

	// Omni-directional point light shadowmapping ////////////////////////////////////////////
	// pushes 6 faces in a single pass
	std::vector<OmniPass> m_OmniPasses;
	void Omni_CreateRenderPasses(uint32_t numPasses);
};

