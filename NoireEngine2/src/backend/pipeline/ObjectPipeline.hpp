#pragma once

#include "VulkanPipeline.hpp"
#include "renderer/PosNorTexVertex.hpp"
#include "backend/buffers/Buffer.hpp"
#include "backend/images/Image2D.hpp"

#include "glm/glm.hpp"
#include <type_traits>

class Renderer;
class MeshRenderInstance;

class ObjectPipeline : public VulkanPipeline
{
public:
	ObjectPipeline(Renderer* renderer);
	virtual ~ObjectPipeline();
public:
	void Render(const CommandBuffer& commandBuffer, uint32_t surfaceId);

	void Update(const Scene* scene) override;

	using Vertex = PosNorTexVertex;

	//types for descriptors:
	struct World {
		struct { float x, y, z, padding_; } SKY_DIRECTION;
		struct { float r, g, b, padding_; } SKY_ENERGY;
		struct { float x, y, z, padding_; } SUN_DIRECTION;
		struct { float r, g, b, padding_; } SUN_ENERGY;
	}world;

	static_assert(sizeof(World) == 16 * 4, "World is the expected size.");

	glm::mat4 projectionMatrix;

	struct TransformUniform {
		glm::mat4 viewMatrix;
		glm::mat4 modelMatrix;
		glm::mat4 modelMatrix_Normal;
	};
	static_assert(sizeof(TransformUniform) == 64 * 3, "Transform Uniform is the expected size.");

private:
	void CreateDescriptors();
	void CreateDescriptorPool();
	void CreatePipeline(VkRenderPass& renderpass, uint32_t subpass) override;
	void PrepareWorkspace();

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

	Buffer vertexBuffer;

	VkDescriptorPool texture_descriptor_pool = VK_NULL_HANDLE;
	std::vector< VkDescriptorSet > texture_descriptors; //allocated from texture_descriptor
	std::vector<std::shared_ptr<Image2D>> textures;

	struct ObjectVertices {
		uint32_t first = 0;
		uint32_t count = 0;
	};
	struct ObjectInstance {
		ObjectVertices vertices;
		TransformUniform transform;
		uint32_t texture = 0;
	};
	ObjectVertices plane_vertices;
	std::vector< ObjectInstance > object_instances;

};

