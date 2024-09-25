#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "utils/Singleton.hpp"
#include "backend/commands/CommandBuffer.hpp"

class Renderer;
class Scene;

class VulkanPipeline : Singleton
{
protected:
	VulkanPipeline() = default;
	virtual ~VulkanPipeline() = default;

public:
	virtual void CreateRenderPass() {}

	virtual void Rebuild() {}

	virtual void CreatePipeline() {}
	
	virtual void Update(const Scene* scene) {}

	virtual void Render(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId) {}

protected:
	VkPipelineLayout			m_PipelineLayout = VK_NULL_HANDLE;
	VkPipeline					m_Pipeline = VK_NULL_HANDLE;
};