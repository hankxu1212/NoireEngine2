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

	void Render(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId) override;

	void Prepare(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId);

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

	VkDescriptorSetLayout set1_CubemapLayout = VK_NULL_HANDLE;
	VkDescriptorSet set1_Cubemap = VK_NULL_HANDLE;

	VkPipeline			m_Pipeline;
	VkPipelineLayout	m_PipelineLayout;
	ObjectPipeline*		p_ObjectPipeline;

	DescriptorAllocator						m_DescriptorAllocator;
	DescriptorLayoutCache					m_DescriptorLayoutCache;

	Buffer m_VertexBuffer;

	std::shared_ptr<ImageCube> cube;
};

