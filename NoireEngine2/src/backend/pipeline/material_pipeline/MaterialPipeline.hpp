#pragma once

#include <vulkan/vulkan.h>

#include "backend/commands/CommandBuffer.hpp"
#include "renderer/materials/Material.hpp"

class Renderer;

class MaterialPipeline
{
public:
	MaterialPipeline() = default;
	
	virtual ~MaterialPipeline();

	void BindPipeline(const CommandBuffer& commandBuffer) const;

	void BindDescriptors(const CommandBuffer& commandBuffer) const;

	virtual void Create() = 0;

	static std::unique_ptr<MaterialPipeline> Create(Material::Workflow workflow);

	inline VkPipelineLayout getPipelineLayout() const { return m_PipelineLayout; }

protected:
	VkPipeline			m_Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout	m_PipelineLayout = VK_NULL_HANDLE;
};

