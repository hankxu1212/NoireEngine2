#pragma once

#include "VulkanPipeline.hpp"
#include "renderer/PosNorTexVertex.hpp"
#include "backend/buffers/Buffer.hpp"
#include "backend/images/Image2D.hpp"
#include "renderer/object/ObjectInstance.hpp"

#include <type_traits>
#include "glm/glm.hpp"

class Renderer;
class MeshRenderInstance;

class ObjectPipeline : public VulkanPipeline
{
public:
	ObjectPipeline(Renderer* renderer);
	virtual ~ObjectPipeline();

	using Vertex = PosNorTexVertex;

public:
	void Render(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId);

	void Update(const Scene* scene) override;

private:
	void CreateDescriptors();
	void CreateDescriptorPool();
	void CreatePipeline(VkRenderPass& renderpass, uint32_t subpass) override;
	void PrepareWorkspace();

	void PushSceneDrawInfo(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId);

private:
	VkDescriptorSetLayout set0_World = VK_NULL_HANDLE;
	VkDescriptorSetLayout set1_Transforms = VK_NULL_HANDLE;
	VkDescriptorSetLayout set2_TEXTURE = VK_NULL_HANDLE;
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

	struct Workspace
	{
		//location for ObjectsPipeline::World data: (streamed to GPU per-frame)
		Buffer World_src; //host coherent; mapped
		Buffer World; //device-local
		VkDescriptorSet World_descriptors; //references World

		//location for ObjectsPipeline::Transforms data: (streamed to GPU per-frame)
		Buffer Transforms_src; //host coherent; mapped
		Buffer Transforms; //device-local
		VkDescriptorSet Transforms_descriptors; //references Transforms
	};
	std::vector<Workspace> workspaces;

	// texture
	VkDescriptorPool texture_descriptor_pool = VK_NULL_HANDLE;
	std::vector< VkDescriptorSet > texture_descriptors; //allocated from texture_descriptor
	std::vector<std::shared_ptr<Image2D>> textures;
};

