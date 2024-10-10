#pragma once

#include <vulkan/vulkan.h>
#include <string>

class VulkanShader
{
public:
	enum ShaderStage { Frag, Vertex, Compute };

public:
	template< size_t N >
	VulkanShader(uint32_t const (&arr)[N], ShaderStage stage) : VulkanShader(arr, 4 * N, stage) {}

	VulkanShader(const uint32_t* code, size_t bytes, ShaderStage stage);

	VulkanShader(const std::string& path, ShaderStage stage);

	~VulkanShader();

	operator const VkShaderModule& () const { return m_ShaderModule; }
	const VkPipelineShaderStageCreateInfo& shaderStage() const{ return m_ShaderStage; }

public:
	VkShaderModule						m_ShaderModule;
	VkPipelineShaderStageCreateInfo		m_ShaderStage;
};