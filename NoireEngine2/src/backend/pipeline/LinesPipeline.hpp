#pragma once

#include "VulkanPipeline.hpp"
#include "backend/renderpass/Renderpass.hpp"
#include "backend/buffers/Buffer.hpp"

class ObjectPipeline;

class LinesPipeline : public VulkanPipeline
{
public:
	LinesPipeline(ObjectPipeline*);
	virtual ~LinesPipeline();

	void CreatePipeline() override;

	void Render(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId) override;

	//descriptor set layouts:
	VkDescriptorSetLayout set0_Camera = VK_NULL_HANDLE;

	//types for descriptors:
	struct Camera {
		glm::mat4 CLIP_FROM_WORLD;
	};
	static_assert(sizeof(Camera) == 64);

	struct Workspace
	{
		Buffer LinesVerticesSrc; //host coherent; mapped
		Buffer LinesVertices; //device-local
	};

	std::vector< Workspace > workspaces;

	VkPipeline			m_Pipeline;
	VkPipelineLayout	m_PipelineLayout;
	ObjectPipeline*		p_ObjectPipeline;
};
