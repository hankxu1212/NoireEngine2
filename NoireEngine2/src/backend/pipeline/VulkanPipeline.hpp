#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "utils/Singleton.hpp"
#include "backend/commands/CommandBuffer.hpp"

class VulkanPipeline : Singleton
{
public:
	// Represents position in the render structure, { renderpass, subpass }
	using Stage = std::pair<uint32_t, uint32_t>;

	void BindPipeline(const CommandBuffer& commandBuffer) const {
		vkCmdBindPipeline(commandBuffer, m_PipelineBindPoint, m_Handle);
	}

	virtual void CreateShaders() {}
	virtual void CreateDescriptors() {}
	virtual void CreatePipeline(VkRenderPass render_pass, uint32_t subpass) {}

public:
	VkPipelineLayout			m_Layout = VK_NULL_HANDLE;
	VkPipeline					m_Handle = VK_NULL_HANDLE;
	VkPipelineBindPoint			m_PipelineBindPoint;

	std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;

};

