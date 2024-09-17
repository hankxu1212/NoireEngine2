#pragma once

#include "VulkanPipeline.hpp"
#include "renderer/PosNorTexVertex.hpp"
#include "backend/buffers/Buffer.hpp"

#include "glm/glm.hpp"
#include <type_traits>

class Renderer;

class ObjectPipeline : public VulkanPipeline
{
public:
	ObjectPipeline(Renderer* renderer);
	~ObjectPipeline();

	void CreateShaders() override;
	void CreateDescriptors() override;
	void CreatePipeline(VkRenderPass& renderpass, uint32_t subpass) override;
	void Render(const CommandBuffer& commandBuffer, uint32_t surfaceId);

	void Update();

	void CreateWorkspaces();

	using Vertex = PosNorTexVertex;

	//types for descriptors:
	struct World {
		struct { float x, y, z, padding_; } SKY_DIRECTION;
		struct { float r, g, b, padding_; } SKY_ENERGY;
		struct { float x, y, z, padding_; } SUN_DIRECTION;
		struct { float r, g, b, padding_; } SUN_ENERGY;
	}world;

	static_assert(sizeof(World) == 4 * 4 + 4 * 4 + 4 * 4 + 4 * 4, "World is the expected size.");

	glm::mat4 CLIP_FROM_WORLD;

	struct Transform {
		glm::mat4 CLIP_FROM_LOCAL;
		glm::mat4 WORLD_FROM_LOCAL;
		glm::mat4 WORLD_FROM_LOCAL_NORMAL;
	};
	static_assert(sizeof(Transform) == 16 * 4 + 16 * 4 + 16 * 4, "Transform is the expected size.");

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

	struct ObjectVertices {
		uint32_t first = 0;
		uint32_t count = 0;
	};
	struct ObjectInstance {
		ObjectVertices vertices;
		Transform transform;
		uint32_t texture = 0;
	};
	ObjectVertices plane_vertices;
	std::vector< ObjectInstance > object_instances;

};

