#include "VulkanShader.h"

#include "backend/VulkanContext.hpp"

VulkanShader::VulkanShader(uint32_t const* code, size_t bytes, ShaderStage stage)
{
	VkShaderModuleCreateInfo create_info
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = bytes,
		.pCode = code
	};

	VulkanContext::VK_CHECK(vkCreateShaderModule(VulkanContext::GetDevice(), &create_info, nullptr, &m_ShaderModule),
		"[Vulkan] Creating shader module failed");

	if (stage == ShaderStage::Frag) {
		m_ShaderStage = VkPipelineShaderStageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = m_ShaderModule,
			.pName = "main"
		};
	}
	else {
		m_ShaderStage = VkPipelineShaderStageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = m_ShaderModule,
			.pName = "main"
		};
	}
}

VulkanShader::~VulkanShader()
{
	vkDestroyShaderModule(VulkanContext::GetDevice(), m_ShaderModule, nullptr);
}
