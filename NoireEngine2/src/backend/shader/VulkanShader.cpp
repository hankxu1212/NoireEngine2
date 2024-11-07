#include "VulkanShader.h"

#include "backend/VulkanContext.hpp"

VulkanShader::VulkanShader(const std::string& path, ShaderStage stage)
{
	std::vector<std::byte> bytes = Files::Read(path);
	VkShaderModuleCreateInfo create_info
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = bytes.size(),
		.pCode = (const uint32_t*)bytes.data()
	};

	VulkanContext::VK(vkCreateShaderModule(VulkanContext::GetDevice(), &create_info, nullptr, &m_ShaderModule),
		"[Vulkan] Creating shader module failed");

	m_ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	m_ShaderStage.module = m_ShaderModule;
	m_ShaderStage.pName = "main";

	switch (stage)
	{
	case VulkanShader::Frag:
		m_ShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		break;
	case VulkanShader::Vertex:
		m_ShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
		break;
	case VulkanShader::Compute:
		m_ShaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		break;
	case VulkanShader::RTX_Raygen:
		m_ShaderStage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		break;
	case VulkanShader::RTX_Miss:
		m_ShaderStage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
		break;
	case VulkanShader::RTX_CHit:
		m_ShaderStage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		break;
	default:
		NE_ERROR("Did not find shader of this stage.");
	}
}

VulkanShader::~VulkanShader()
{
	vkDestroyShaderModule(VulkanContext::GetDevice(), m_ShaderModule, nullptr);
}
