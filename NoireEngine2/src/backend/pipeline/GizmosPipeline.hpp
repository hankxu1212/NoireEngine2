#pragma once

#include "VulkanPipeline.hpp"
#include "backend/renderpass/Renderpass.hpp"
#include "backend/buffers/Buffer.hpp"
#include "renderer/vertices/PosColVertex.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"
#include "renderer/object/ObjectInstance.hpp"
#include "core/Core.hpp"

class Renderer;

class GizmosPipeline : public VulkanPipeline
{
public:
	~GizmosPipeline();

	void CreatePipeline() override;

	void Render(const Scene* scene, const CommandBuffer& commandBuffer) override;

	void RenderLines(const Scene* scene, const CommandBuffer& commandBuffer);
	void RenderOutline(const Scene* scene, const CommandBuffer& commandBuffer);

	void Prepare(const Scene* scene, const CommandBuffer& commandBuffer);

private:
	void CreateLinesGraphicsPipeline();
	void CreateLinesPipelineLayout();
	void CreateLinesDescriptors();

	void CreateOutlineGraphicsPipeline();
	void CreateOutlinePipelineLayout();
	void CreateOutlineDescriptors();


	//types for descriptors:
	struct CameraUniform {
		glm::mat4 clipFromWorld;
	} pushCamera;
	static_assert(sizeof(CameraUniform) == 64);

	struct Workspace
	{
		Buffer LinesVerticesSrc;
		Buffer LinesVertices;

		Buffer Camera;

		Buffer TransformsSrc;
		Buffer Transforms;

		VkDescriptorSet set0;
	};

	START_BINDING(Set0)
		World,
		Transforms, // g buffer color sampler
	END_BINDING();

	std::vector<Workspace> workspaces;

	VkDescriptorSetLayout set0_Layout = VK_NULL_HANDLE;

	VkPipeline			m_LinesPipeline = VK_NULL_HANDLE;
	VkPipelineLayout	m_LinesPipelineLayout = VK_NULL_HANDLE;

	VkPipeline			m_OutlinePipeline = VK_NULL_HANDLE;
	VkPipelineLayout	m_OutlinePipelineLayout = VK_NULL_HANDLE;

	DescriptorAllocator m_DescriptorAllocator;
};
