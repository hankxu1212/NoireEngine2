#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "utils/Singleton.hpp"
#include "backend/commands/CommandBuffer.hpp"

class Renderer;

class VulkanPipeline : Singleton
{
protected:
	VulkanPipeline(Renderer* renderer) : m_BindedRenderer(renderer) {}
	virtual ~VulkanPipeline() = default;
public:
	virtual void CreateShaders() {}
	virtual void CreateDescriptors() {}
	virtual void CreatePipeline(VkRenderPass& renderpass, uint32_t subpass) {}
	
	virtual void Update() {}

	VkPipelineLayout			m_Layout = VK_NULL_HANDLE;
	VkPipeline					m_Pipeline = VK_NULL_HANDLE;

	std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;

protected:
	Renderer* m_BindedRenderer;
};

