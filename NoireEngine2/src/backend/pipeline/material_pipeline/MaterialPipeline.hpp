#pragma once

#include <vulkan/vulkan.h>

#include "backend/commands/CommandBuffer.hpp"
#include "renderer/materials/Material.hpp"

class Renderer;

class MaterialPipeline
{
public:
	MaterialPipeline() = default;
	MaterialPipeline(Renderer* renderer);
	
	virtual ~MaterialPipeline();

	void BindPipeline(const CommandBuffer& commandBuffer);

	virtual void Create() {}

	virtual void BindDescriptors(const CommandBuffer& commandBuffer) {}

	static std::unique_ptr<MaterialPipeline> Create(Material::Workflow workflow, Renderer* renderer);

	VkPipeline			m_Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout	m_PipelineLayout = VK_NULL_HANDLE;
	Renderer*		p_ObjectPipeline = nullptr;
};

