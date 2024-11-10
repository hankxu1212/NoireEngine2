#pragma once

#include "backend/pipeline/VulkanPipeline.hpp"
#include "backend/buffers/Buffer.hpp"
#include "backend/images/Image2D.hpp"
#include "renderer/object/ObjectInstance.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"
#include "backend/renderpass/Renderpass.hpp"

#include "backend/pipeline/SkyboxPipeline.hpp"
#include "backend/pipeline/LinesPipeline.hpp"
#include "backend/pipeline/ShadowPipeline.hpp"
#include "backend/pipeline/RaytracingPipeline.hpp"
#include "backend/pipeline/ImGuiPipeline.hpp"

#include <type_traits>
#include "glm/glm.hpp"

#include "utils/ThreadPool.hpp"

#define _NE_USE_RTX

/**
 * Manages all the material pipelines and the main geometry render pass.
 */
class Renderer : Singleton
{
public:
	inline static Renderer* Instance;

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
	void CreateMaterialPipelineLayout();
	void CreateMaterialPipelines();

	void CreateDescriptors();
	void CreateWorkspaceDescriptors();
	void CreateTextureDescriptors();
	void CreateIBLDescriptors();
	void CreateShadowDescriptors();
	void CreateRayTracingDescriptors();

	void Prepare(const Scene* scene, const CommandBuffer& commandBuffer);
	void PrepareSceneUniform(const Scene* scene, const CommandBuffer& commandBuffer);
	void PrepareTransforms(const Scene* scene, const CommandBuffer& commandBuffer);
	void PrepareLights(const Scene* scene, const CommandBuffer& commandBuffer);
	void PrepareMaterialInstances(const CommandBuffer& commandBuffer);
	void PrepareObjectDescriptions(const Scene* scene, const CommandBuffer& commandBuffer);

	void CompactDraws(const std::vector<ObjectInstance>& objects, uint32_t workflowIndex);

	void PrepareIndirectDrawBuffer(const Scene* scene);

	void DrawScene(const Scene* scene, const CommandBuffer& commandBuffer);

	struct ObjectDescription
	{
		uint64_t vertexAddress;         // Address of the Vertex buffer
		uint64_t indexAddress;          // Address of the index buffer
		uint32_t materialOffset; // where to look in the material buffer
		uint32_t materialWorkflow; // interpret the buffer data as which material type?
	};

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

		// object descriptions
		Buffer ObjectDescriptionsSrc;
		Buffer ObjectDescriptions;

		// all storage buffers are binded to this
		VkDescriptorSet set1_StorageBuffers = VK_NULL_HANDLE; //references Transforms and lights
	};

	START_BINDING(StorageBuffers)
		Transform = 0,
		DirLights = 1,
		PointLights = 2,
		SpotLights = 3,
		Materials = 4,
		Objects = 5
	END_BINDING();

	START_BINDING(IBL)
		Skybox = 0,
		LambertianLUT = 1,
		SpecularBRDF = 2,
		EnvMap = 3,
	END_BINDING();

	START_BINDING(RTXBindings)
		TLAS = 0,  // Top-level acceleration structure
		OutImage = 1,   // Ray tracer output image
	END_BINDING();

	std::vector<Workspace> workspaces;

	VkDescriptorSetLayout set0_WorldLayout = VK_NULL_HANDLE;
	
	VkDescriptorSetLayout set1_StorageBuffersLayout = VK_NULL_HANDLE;

	VkDescriptorSetLayout set2_TexturesLayout = VK_NULL_HANDLE;
	VkDescriptorSet set2_Textures = VK_NULL_HANDLE;

	VkDescriptorSetLayout set3_IBLLayout = VK_NULL_HANDLE;
	VkDescriptorSet set3_IBL = VK_NULL_HANDLE;

	VkDescriptorSetLayout set4_ShadowMapLayout = VK_NULL_HANDLE;
	VkDescriptorSet set4_ShadowMap = VK_NULL_HANDLE;

	VkDescriptorSetLayout set5_RayTracingLayout = VK_NULL_HANDLE;
	VkDescriptorSet set5_RayTracing = VK_NULL_HANDLE;

	DescriptorAllocator						m_DescriptorAllocator;

	std::unique_ptr<Renderpass>				m_Renderpass;

	std::vector<std::vector<IndirectBatch>> m_IndirectBatches;

	std::array<VkPipeline, N_MATERIAL_WORKFLOWS>	m_MaterialPipelines{};
	VkPipelineLayout	m_MaterialPipelineLayout = VK_NULL_HANDLE;

	std::unique_ptr<Image2D>				m_RtxImage;

private:
	friend class LinesPipeline;
	friend class SkyboxPipeline;
	friend class ShadowPipeline;
	friend class RaytracingPipeline;
	friend class ImGuiPipeline;

private: // material pipelines
	std::unique_ptr<LinesPipeline>					s_LinesPipeline;
	std::unique_ptr<SkyboxPipeline>					s_SkyboxPipeline;
	std::unique_ptr<ShadowPipeline>					s_ShadowPipeline;
	std::unique_ptr<RaytracingPipeline>				s_RaytracingPipeline;
	std::unique_ptr<ImGuiPipeline>					s_UIPipeline;
};

