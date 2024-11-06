#pragma once

#include <vulkan/vulkan.h>
#include <string>

class VulkanShader
{
public:
	enum ShaderStage { Frag, Vertex, Compute, RTX_Raygen, RTX_Miss, RTX_CHit };

public:
	VulkanShader(const std::string& path, ShaderStage stage);

	~VulkanShader();

	operator const VkShaderModule& () const { return m_ShaderModule; }
	const VkPipelineShaderStageCreateInfo& shaderStage() const{ return m_ShaderStage; }

private:
	VkShaderModule						m_ShaderModule;
	VkPipelineShaderStageCreateInfo		m_ShaderStage{};
};