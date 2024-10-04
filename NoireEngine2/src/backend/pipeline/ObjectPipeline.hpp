#pragma once

#include "VulkanPipeline.hpp"
#include "backend/buffers/Buffer.hpp"
#include "backend/images/Image2D.hpp"
#include "renderer/object/ObjectInstance.hpp"
#include "backend/images/ImageDepth.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"
#include "backend/pipeline/material_pipeline/MaterialPipeline.hpp"

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

public:
	void CreateRenderPass() override;

	void Rebuild() override;

	void CreatePipeline() override;

	void Update(const Scene* scene) override;

	void Render(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId) override;

private:
	void CreateDescriptors();
	
	void PushSceneDrawInfo(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId);

	void RenderPass(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId);

	void DestroyFrameBuffers();

	struct Workspace
	{
		//location for ObjectsPipeline::World data: (streamed to GPU per-frame)
		Buffer World_src; //host coherent; mapped
		Buffer World; //device-local
		VkDescriptorSet set0_World; //references World

		//location for ObjectsPipeline::Transforms data: (streamed to GPU per-frame)
		Buffer Transforms_src; //host coherent; mapped
		Buffer Transforms; //device-local
		VkDescriptorSet set1_Transforms; //references Transforms
	};

	VkDescriptorBufferInfo CreateTransformStorageBuffer(Workspace& workspace, size_t new_bytes);

private:
	friend class LambertianMaterialPipeline;

	VkDescriptorSetLayout set0_WorldLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout set1_TransformsLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout set2_TexturesLayout = VK_NULL_HANDLE;

	std::vector<Workspace> workspaces;

	// texture
	VkDescriptorSet set2_Textures; //allocated from texture_descriptor
	std::vector<std::shared_ptr<Image2D>>	textures;

	DescriptorAllocator						m_DescriptorAllocator;
	DescriptorLayoutCache					m_DescriptorLayoutCache;

	VkRenderPass							m_Renderpass = VK_NULL_HANDLE;
	std::unique_ptr<ImageDepth>				s_SwapchainDepthImage;
	std::vector<VkFramebuffer>				m_Framebuffers;

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
	std::vector<std::unique_ptr<MaterialPipeline>> m_MaterialPipelines;

};

