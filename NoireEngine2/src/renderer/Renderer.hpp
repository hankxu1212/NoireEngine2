#pragma once

#include "backend/pipeline/VulkanPipeline.hpp"
#include "backend/buffers/Buffer.hpp"
#include "backend/images/Image2D.hpp"
#include "renderer/object/ObjectInstance.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"
#include "backend/renderpass/Renderpass.hpp"

#include "backend/pipeline/material_pipeline/MaterialPipeline.hpp"
#include "backend/pipeline/SkyboxPipeline.hpp"
#include "backend/pipeline/LinesPipeline.hpp"
#include "backend/pipeline/ShadowPipeline.hpp"
#include "backend/pipeline/RaytracingPipeline.hpp"
#include "backend/pipeline/ImGuiPipeline.hpp"

#include <type_traits>
#include "glm/glm.hpp"

#include "utils/ThreadPool.hpp"

class Renderer;
class MeshRenderInstance;

/**
 * Manages all the material pipelines and the main geometry render pass.
 */
class Renderer : Singleton
{
public:
	Renderer();
	~Renderer();

	// UI statistics
	inline static size_t ObjectsDrawn, VerticesDrawn, NumDrawCalls;
	inline static bool UseGizmos = false;

public:
	void CreateRenderPass();

	void Rebuild();

	void Create();

	void Update();

	void Render(const CommandBuffer& commandBuffer);

	// for draw indirect
	struct IndirectBatch
	{
		Mesh* mesh;
		Material* material;
		uint32_t firstInstanceIndex;
		uint32_t count;
	};

	const std::vector<std::vector<IndirectBatch>>& getIndirectBatches() const { return m_IndirectBatches; }

private:
	void CreateDescriptors();
	void CreateWorkspaceDescriptors();
	void CreateTextureDescriptors();
	void CreateCubemapDescriptors();
	void CreateShadowDescriptors();

	void Prepare(const Scene* scene, const CommandBuffer& commandBuffer);
	void PrepareSceneUniform(const Scene* scene, const CommandBuffer& commandBuffer);
	void PrepareTransforms(const Scene* scene, const CommandBuffer& commandBuffer);
	void PrepareLights(const Scene* scene, const CommandBuffer& commandBuffer);


	void CompactDraws(const std::vector<ObjectInstance>& objects, uint32_t workflowIndex);

	void PrepareIndirectDrawBuffer(const Scene* scene);

	void DrawScene(const Scene* scene, const CommandBuffer& commandBuffer);

	struct Workspace
	{
		// world, scene uniform
		Buffer WorldSrc; //host coherent; mapped
		Buffer World; //device-local
		VkDescriptorSet set0_World = VK_NULL_HANDLE;

		// transforms
		Buffer TransformsSrc; //host coherent; mapped
		Buffer Transforms; //device-local

		// light list by type
		std::array<Buffer, 3> LightsSrc;
		std::array<Buffer, 3> Lights;

		// material instances
		Buffer MaterialInstancesSrc;
		Buffer MaterialInstances;

		VkDescriptorSet set1_StorageBuffers = VK_NULL_HANDLE; //references Transforms and lights
	};

private:
	friend class LambertianMaterialPipeline;
	friend class EnvironmentMaterialPipeline;
	friend class MirrorMaterialPipeline;
	friend class PBRMaterialPipeline;
	friend class LinesPipeline;
	friend class SkyboxPipeline;
	friend class ShadowPipeline;
	friend class RaytracingPipeline;
	friend class ImGuiPipeline;

	std::vector<Workspace> workspaces;

	VkDescriptorSetLayout set0_WorldLayout = VK_NULL_HANDLE;
	
	VkDescriptorSetLayout set1_StorageBuffersLayout = VK_NULL_HANDLE;

	VkDescriptorSetLayout set2_TexturesLayout = VK_NULL_HANDLE;
	VkDescriptorSet set2_Textures = VK_NULL_HANDLE;

	VkDescriptorSetLayout set3_CubemapLayout = VK_NULL_HANDLE;
	VkDescriptorSet set3_Cubemap = VK_NULL_HANDLE;

	VkDescriptorSetLayout set4_ShadowMapLayout = VK_NULL_HANDLE;
	VkDescriptorSet set4_ShadowMap = VK_NULL_HANDLE;

	DescriptorAllocator						m_DescriptorAllocator;

	std::unique_ptr<Renderpass>				m_Renderpass;

	std::vector<std::vector<IndirectBatch>> m_IndirectBatches;

private: // material pipelines
	std::vector<std::unique_ptr<MaterialPipeline>>	m_MaterialPipelines;
	std::unique_ptr<LinesPipeline>					s_LinesPipeline;
	std::unique_ptr<SkyboxPipeline>					s_SkyboxPipeline;
	std::unique_ptr<ShadowPipeline>					s_ShadowPipeline;
	std::unique_ptr<RaytracingPipeline>				s_RaytracingPipeline;
	std::unique_ptr<ImGuiPipeline>					s_UIPipeline;
};

