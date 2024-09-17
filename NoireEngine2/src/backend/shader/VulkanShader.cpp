#include "VulkanShader.h"

#include "backend/VulkanContext.hpp"

VulkanShader::VulkanShader(uint32_t const* code, size_t bytes)
{
	VkShaderModuleCreateInfo create_info
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = bytes,
		.pCode = code
	};

	VulkanContext::VK_CHECK(vkCreateShaderModule(VulkanContext::GetDevice(), &create_info, nullptr, &m_ShaderModule),
		"[Vulkan] Creating shader module failed");
}

VulkanShader::~VulkanShader()
{
	vkDestroyShaderModule(VulkanContext::GetDevice(), m_ShaderModule, nullptr);
}
