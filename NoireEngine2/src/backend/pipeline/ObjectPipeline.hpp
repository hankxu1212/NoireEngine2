#pragma once

#include "VulkanPipeline.hpp"
#include "backend/buffers/Buffer.hpp"
#include "backend/images/Image2D.hpp"
#include "renderer/object/ObjectInstance.hpp"
#include "backend/images/ImageDepth.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"
#include "backend/pipeline/material_pipeline/MaterialPipeline.hpp"
#include "LinesPipeline.hpp"
#include "backend/renderpass/Renderpass.hpp"

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
	inline static bool UseGizmos = true;

public:
	void CreateRenderPass() override;

	void Rebuild() override;

	void CreatePipeline() override;

	void Update(const Scene* scene) override;

	void Render(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId) override;

private:
	void CreateDescriptors();
	
	void Prepare(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId);

	void RenderPass(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId);

	struct Workspace
	{
		Buffer WorldPersistent; //host coherent and visible; persistently mapped
		VkDescriptorSet set0_World;

		//location for ObjectsPipeline::Transforms data: (streamed to GPU per-frame)
		Buffer TransformsPersistent; // host hoherent and visible, persistently mapped
		VkDescriptorSet set1_Transforms; //references Transforms
	};

	VkDescriptorBufferInfo CreateTransformStorageBuffer(Workspace& workspace, size_t new_bytes);

private:
	friend class LambertianMaterialPipeline;
	friend class LinesPipeline;

	VkDescriptorSetLayout set0_WorldLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout set1_TransformsLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout set2_TexturesLayout = VK_NULL_HANDLE;

	std::vector<Workspace> workspaces;

	VkDescriptorSet set2_Textures;

	DescriptorAllocator						m_DescriptorAllocator;
	DescriptorLayoutCache					m_DescriptorLayoutCache;

	std::unique_ptr<Renderpass>				m_Renderpass;

	// for draw indirect
	struct IndirectBatch
	{
		Mesh* mesh;
		Material* material;
		uint32_t first;
		uint32_t count;
	};

	std::vector<IndirectBatch> CompactDraws(const std::vector<ObjectInstance>& objects);

private: // material pipelines
	std::vector<std::unique_ptr<MaterialPipeline>>	m_MaterialPipelines;
	std::unique_ptr<LinesPipeline>					s_LinesPipeline;
};

