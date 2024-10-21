#pragma once

#include "VulkanPipeline.hpp"
#include "backend/buffers/Buffer.hpp"
#include "backend/images/Image2D.hpp"
#include "renderer/object/ObjectInstance.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"
#include "backend/renderpass/Renderpass.hpp"

#include "backend/pipeline/material_pipeline/MaterialPipeline.hpp"
#include "SkyboxPipeline.hpp"
#include "LinesPipeline.hpp"
#include "ShadowPipeline.hpp"

#include <type_traits>
#include "glm/glm.hpp"

class Renderer;
class MeshRenderInstance;

/**
 * Manages all the material pipelines and the main geometry render pass.
 */
class ObjectPipeline : public VulkanPipeline
{
public:
	ObjectPipeline();
	virtual ~ObjectPipeline();

	inline static size_t ObjectsDrawn, VerticesDrawn, NumDrawCalls;
	inline static bool UseGizmos = false;

public:
	void CreateRenderPass() override;

	void Rebuild() override;

	void CreatePipeline() override;

	void Update(const Scene* scene) override;

	void Render(const Scene* scene, const CommandBuffer& commandBuffer) override;

	// for draw indirect
	struct IndirectBatch
	{
		Mesh* mesh;
		Material* material;
		uint32_t firstInstanceIndex;
		uint32_t count;
	};
	static std::vector<IndirectBatch> CompactDraws(const std::vector<ObjectInstance>& objects);

private:
	void CreateDescriptors();
	
	void Prepare(const Scene* scene, const CommandBuffer& commandBuffer);

	void DrawScene(const Scene* scene, const CommandBuffer& commandBuffer);

	struct Workspace
	{
		//location for ObjectsPipeline::World data: (streamed to GPU per-frame)
		Buffer World_src; //host coherent; mapped
		Buffer World; //device-local
		VkDescriptorSet set0_World; //references World

		//location for ObjectsPipeline::Transforms data: (streamed to GPU per-frame)
		Buffer Transforms_src; //host coherent; mapped
		Buffer Transforms; //device-local

		std::array<Buffer, 3> Lights_src;
		std::array<Buffer, 3> Lights;

		VkDescriptorSet set1_StorageBuffers; //references Transforms and lights
	};

private:
	friend class LambertianMaterialPipeline;
	friend class EnvironmentMaterialPipeline;
	friend class MirrorMaterialPipeline;
	friend class PBRMaterialPipeline;
	friend class LinesPipeline;
	friend class SkyboxPipeline;

	std::vector<Workspace> workspaces;

	VkDescriptorSetLayout set0_WorldLayout = VK_NULL_HANDLE;
	
	VkDescriptorSetLayout set1_StorageBuffersLayout = VK_NULL_HANDLE;

	VkDescriptorSetLayout set2_TexturesLayout = VK_NULL_HANDLE;
	VkDescriptorSet set2_Textures;

	VkDescriptorSetLayout set3_CubemapLayout = VK_NULL_HANDLE;
	VkDescriptorSet set3_Cubemap = VK_NULL_HANDLE;

	VkDescriptorSetLayout set4_ShadowMapLayout = VK_NULL_HANDLE;
	VkDescriptorSet set4_ShadowMap = VK_NULL_HANDLE;

	DescriptorAllocator						m_DescriptorAllocator;

	std::unique_ptr<Renderpass>				m_Renderpass;

private: // material pipelines
	std::vector<std::unique_ptr<MaterialPipeline>>	m_MaterialPipelines;
	std::unique_ptr<LinesPipeline>					s_LinesPipeline;
	std::unique_ptr<SkyboxPipeline>					s_SkyboxPipeline;
	std::unique_ptr<ShadowPipeline>					s_ShadowPipeline;
};

