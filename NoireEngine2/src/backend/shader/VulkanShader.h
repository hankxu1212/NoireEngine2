#pragma once

#include <vulkan/vulkan.h>

class VulkanShader
{
public:
	enum ShaderStage { Frag, Vertex };

public:
	template< size_t N >
	VulkanShader(uint32_t const (&arr)[N], ShaderStage stage) : VulkanShader(arr, 4 * N, stage) {}

	VulkanShader(uint32_t const* code, size_t bytes, ShaderStage stage);

	~VulkanShader();

	operator const VkShaderModule& () const { return m_ShaderModule; }
	const VkPipelineShaderStageCreateInfo& shaderStage() const{ return m_ShaderStage; }

public:
	VkShaderModule						m_ShaderModule;
	VkPipelineShaderStageCreateInfo		m_ShaderStage;
};