#pragma once

#include "backend/pipeline/VulkanPipeline.hpp"
#include "backend/buffers/Buffer.hpp"
#include "backend/images/Image2D.hpp"
#include "renderer/object/ObjectInstance.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"
#include "backend/renderpass/Renderpass.hpp"

#include "renderer/materials/Materials.hpp"

#include "backend/pipeline/SkyboxPipeline.hpp"
#include "backend/pipeline/GizmosPipeline.hpp"
#include "backend/pipeline/ShadowPipeline.hpp"
#include "backend/pipeline/ReflectionPipeline.hpp"
#include "backend/pipeline/UIPipeline.hpp"
#include "backend/pipeline/BloomPipeline.hpp"
#include "backend/pipeline/TransparencyPipeline.hpp"

#include <type_traits>
#include "glm/glm.hpp"

#include "utils/ThreadPool.hpp"
#include "utils/UUID.hpp"

#define _NE_USE_RTX
#define N_OPAQUE_MATERIALS 2

class Renderer : Singleton
{
public:
	inline static Renderer* Instance;

	Renderer();
	~Renderer();

	// UI statistics
	inline static size_t ObjectsDrawn, VerticesDrawn, NumDrawCalls;
	inline static bool UseGizmos = true;
	inline static bool DrawSkybox = true;

public:
	void CreateRenderPass();

	void Rebuild();

	void Create();

	void Update();

	void Render(const CommandBuffer& commandBuffer);
	
	UUID QueryMouseHoveredEntity() const;

	// for draw indirect
	struct IndirectBatch
	{
		Mesh* mesh;
		Material* material;
		uint32_t firstInstanceIndex;
		uint32_t count;
	};

	const std::vector<std::vector<IndirectBatch>>& getIndirectBatches() const { return m_IndirectBatches; }

	void OnUIRender();
	VkRenderPass GetUIRenderPass() { return s_UIPipeline->GetRenderPass(); }

private:
	void AddUIViewportImages();

	// material pipelines
	void CreateMaterialPipelineLayout();
	void CreateMaterialPipelines();

	void CreatePostPipelineLayout();
	void CreatePostPipeline();

	void CreateComputeAOPipeline();

	// descriptor management
	Buffer m_MousePicking;

	void CreateDescriptors();
	bool createdDescriptors = false;

	void CreateWorldDescriptors(bool update);
	void CreateStorageBufferDescriptors();
	void CreateTextureDescriptors();
	void CreateIBLDescriptors();
	void CreateShadowDescriptors();
	void CreateRayTracingImages();
	void CreateRaytracingDescriptors(bool update);

	// prepping and copy/update buffers
	void Prepare(const Scene* scene, const CommandBuffer& commandBuffer);
	void PrepareSceneUniform(const Scene* scene, const CommandBuffer& commandBuffer);
	void PrepareTransforms(const Scene* scene, const CommandBuffer& commandBuffer);
	void PrepareLights(const Scene* scene, const CommandBuffer& commandBuffer);
	void PrepareMaterialInstances(const CommandBuffer& commandBuffer);
	void PrepareObjectDescriptions(const Scene* scene, const CommandBuffer& commandBuffer);

	struct ObjectDescription
	{
		uint64_t vertexAddress;         // Address of the Vertex buffer
		uint64_t indexAddress;          // Address of the index buffer
		uint32_t materialOffset; // where to look in the material buffer
		uint32_t materialWorkflow; // interpret the buffer data as which material type?
		uint64_t entityID;
	};

	// drawing
	void DrawScene(const Scene* scene, const CommandBuffer& commandBuffer);
	void CompactDraws(const std::vector<ObjectInstance>& objects, uint32_t workflowIndex);
	void PrepareIndirectDrawBuffer(const Scene* scene);
	std::vector<std::vector<IndirectBatch>> m_IndirectBatches;

	// dispatch ray queries from compute shader
	void RunAOCompute(const Scene* scene, const CommandBuffer& commandBuffer);

	// run ray tracing pipeline for reflection
	void RunRTXReflection(const Scene* scene, const CommandBuffer& commandBuffer);

	// run ray tracing pipeline for transparency
	void RunRTXTransparency(const Scene* scene, const CommandBuffer& commandBuffer);

	// composition pass, to compose ambient occlusion and color from G buffer and emission
	void RunPost(const CommandBuffer& commandBuffer);

	// workspace and bindings: these are streamed to GPU per frame
	struct Workspace
	{
		// world, scene uniform
		Buffer WorldSrc; //host coherent; mapped
		Buffer World; //device-local

		std::unique_ptr<Image2D> GBufferNormals;
		std::unique_ptr<Image2D> GBufferColors;
		std::unique_ptr<Image2D> GBufferEmission;

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

	std::vector<Workspace> workspaces;

#pragma region Bindings

	START_BINDING(World)
		SceneUniform,
		GBufferColor, // g buffer color sampler
		GBufferNormal,
		GBufferEmissive,
	END_BINDING();

	START_BINDING(StorageBuffers)
		Transform,
		DirLights,
		PointLights,
		SpotLights,
		Materials,
		Objects,
		MousePicking
	END_BINDING();

	START_BINDING(IBL)
		Skybox,
		LambertianLUT,
		SpecularBRDF,
		EnvMap,
	END_BINDING();

	START_BINDING(RTXBindings)
		TLAS,  // Top-level acceleration structure
		ReflectionImage,   // reflection
		AOImage, // ao image
		TransparencyImage
	END_BINDING();

#pragma endregion

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

	DescriptorAllocator m_DescriptorAllocator;

	// render pass management
	std::unique_ptr<Renderpass> s_OffscreenPass;
	std::unique_ptr<Renderpass> s_CompositionPass;

	// frame buffers
	std::vector<VkFramebuffer> m_CompositionFrameBuffers;
	std::vector<VkFramebuffer> m_OffscreenFrameBuffers;

	void CreateFrameBuffers();
	void DestroyFrameBuffers();

	// images: depth, ray tracing
	std::unique_ptr<ImageDepth> s_MainDepth;
	std::unique_ptr<Image2D> s_RaytracedAOImage;
	std::unique_ptr<Image2D> s_RaytracedReflectionsImage;
	std::unique_ptr<Image2D> s_RaytracedTransparencyImage;

	// post pipeline
	struct PostPush
	{
		int useToneMapping = 1;
		int useBloom = 1;
		float bloomStrength = 0.1f;
		int useRaytracedAO = 1;
	}m_PostPush;

	VkPipeline m_PostPipeline = VK_NULL_HANDLE;
	VkPipelineLayout m_PostPipelineLayout = VK_NULL_HANDLE;

	// ray tracing AO pipeline
	struct AOPush
	{
		float radius = 2.0f;       // Length of the ray
		int samples = 4;         // Nb samples at each iteration
		float power = 3;        // Darkness is stronger for more hits
		int distanceBased = 1;  // Attenuate based on distance
		int frame = 0;                // Current frame
		int maxSamples = 100'000;    // Max samples before it stops
	}m_AOControl;

	bool m_AOIsDirty = false;

	VkPipeline m_RaytracedAOComputePipeline = VK_NULL_HANDLE;
	VkPipelineLayout m_RaytracedAOComputePipelineLayout = VK_NULL_HANDLE;

	// material pipelines
	std::array<VkPipeline, N_OPAQUE_MATERIALS> m_MaterialPipelines{};
	VkPipelineLayout m_MaterialPipelineLayout = VK_NULL_HANDLE;

private:
	friend class GizmosPipeline;
	friend class SkyboxPipeline;
	friend class ShadowPipeline;
	friend class ReflectionPipeline;
	friend class UIPipeline;
	friend class BloomPipeline;
	friend class TransparencyPipeline;

private:
	std::unique_ptr<GizmosPipeline>					s_GizmosPipeline;
	std::unique_ptr<SkyboxPipeline>					s_SkyboxPipeline;
	std::unique_ptr<ShadowPipeline>					s_ShadowPipeline;
	std::unique_ptr<ReflectionPipeline>				s_ReflectionPipeline;
	std::unique_ptr<UIPipeline>						s_UIPipeline;
	std::unique_ptr<BloomPipeline>					s_BloomPipeline;
	std::unique_ptr<TransparencyPipeline>			s_TransparencyPipeline;
};

