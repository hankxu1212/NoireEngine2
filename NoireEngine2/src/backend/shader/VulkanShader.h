#pragma once

#include <vulkan/vulkan.h>

class VulkanShader
{
public:
	template< size_t N >
	VulkanShader(uint32_t const (&arr)[N]) : VulkanShader(arr, 4 * N) {}

	VulkanShader(uint32_t const* code, size_t bytes);
	~VulkanShader();

	operator const VkShaderModule& () const { return m_ShaderModule; }

public:
	VkShaderModule m_ShaderModule;
};