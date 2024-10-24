#pragma once

#include "VulkanPipeline.hpp"
#include "backend/renderpass/Renderpass.hpp"
#include "backend/buffers/Buffer.hpp"
#include "renderer/vertices/PosColVertex.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"

class ObjectPipeline;

class LinesPipeline : public VulkanPipeline
{
public:
	LinesPipeline(ObjectPipeline*);
	~LinesPipeline();

	void CreatePipeline() override;

	void Render(const Scene* scene, const CommandBuffer& commandBuffer) override;

	void Prepare(const Scene* scene, const CommandBuffer& commandBuffer);

private:
	void CreateGraphicsPipeline();
	void CreatePipelineLayout();
	void CreateDescriptors();

	VkDescriptorSetLayout set0_CameraLayout = VK_NULL_HANDLE;

	//types for descriptors:
	struct CameraUniform {
		glm::mat4 clipFromWorld;
	};
	static_assert(sizeof(CameraUniform) == 64);

	struct Workspace
	{
		Buffer LinesVertices; //device-local
		Buffer LinesVerticesSrc; //host coherent; mapped

		//location for LinesPipeline::Camera data: (streamed to GPU per-frame)
		Buffer CameraSrc; //host coherent; mapped
		Buffer Camera; //device-local
		VkDescriptorSet set0_Camera; //references Camera
	};

	std::vector<Workspace> workspaces;

	VkPipeline			m_Pipeline;
	VkPipelineLayout	m_PipelineLayout;
	ObjectPipeline*		p_ObjectPipeline;

	DescriptorAllocator						m_DescriptorAllocator;
};
