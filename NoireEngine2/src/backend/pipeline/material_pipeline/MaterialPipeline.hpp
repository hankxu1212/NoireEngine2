#pragma once

#include <vulkan/vulkan.h>

#include "backend/commands/CommandBuffer.hpp"
#include "renderer/materials/Material.hpp"

class ObjectPipeline;

class MaterialPipeline
{
public:
	MaterialPipeline() = default;
	MaterialPipeline(ObjectPipeline* objectPipeline);
	
	~MaterialPipeline();

	void BindPipeline(const CommandBuffer& commandBuffer);

	virtual void Create() {}

	virtual void BindDescriptors(const CommandBuffer& commandBuffer, Material* materialInstance) {}

	static std::unique_ptr<MaterialPipeline> Create(Material::Workflow workflow, ObjectPipeline* objectPipeline);

	VkPipeline			m_Pipeline;
	VkPipelineLayout	m_PipelineLayout;
	ObjectPipeline*		p_ObjectPipeline;
};

