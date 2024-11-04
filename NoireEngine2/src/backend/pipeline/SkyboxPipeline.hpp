#pragma once

#include "VulkanPipeline.hpp"
#include "backend/renderpass/Renderpass.hpp"
#include "backend/buffers/Buffer.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"
#include "backend/images/ImageCube.hpp"

class ObjectPipeline;

class SkyboxPipeline : public VulkanPipeline
{
public:
	SkyboxPipeline(ObjectPipeline*);

	~SkyboxPipeline();
	void CreatePipeline() override;

	void Render(const Scene* scene, const CommandBuffer& commandBuffer) override;

	void Prepare(const Scene* scene, const CommandBuffer& commandBuffer);

private:
	void CreateGraphicsPipeline();
	void CreatePipelineLayout();
	void CreateDescriptors();
	void CreateVertexBuffer();

	VkDescriptorSetLayout set0_CameraLayout = VK_NULL_HANDLE;

	struct CameraUniform {
		glm::mat4 projection;
		glm::mat4 view;
	};
	static_assert(sizeof(CameraUniform) == 64 * 2);

	struct Workspace
	{
		//location for SkyboxPipeline::Camera data: (streamed to GPU per-frame)
		Buffer CameraSrc; //host coherent; mapped
		Buffer Camera; //device-local
		VkDescriptorSet set0_Camera; //references Camera
	};
	std::vector<Workspace> workspaces;

	VkPipeline			m_Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout	m_PipelineLayout = VK_NULL_HANDLE;
	ObjectPipeline*		p_ObjectPipeline;

	DescriptorAllocator						m_DescriptorAllocator;

	Buffer m_VertexBuffer;
};

