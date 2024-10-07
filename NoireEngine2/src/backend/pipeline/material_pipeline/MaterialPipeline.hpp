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
	
	virtual ~MaterialPipeline();

	void BindPipeline(const CommandBuffer& commandBuffer);

	virtual void Create() {}

	virtual void BindDescriptors(const CommandBuffer& commandBuffer, uint32_t surfaceId) {}

	static std::unique_ptr<MaterialPipeline> Create(Material::Workflow workflow, ObjectPipeline* objectPipeline);

	VkPipeline			m_Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout	m_PipelineLayout = VK_NULL_HANDLE;
	ObjectPipeline*		p_ObjectPipeline = nullptr;
};

