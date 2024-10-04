#pragma once

#include "VulkanPipeline.hpp"
#include "backend/images/ImageDepth.hpp"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/backends/imgui_impl_glfw.h"

class ImGuiPipeline : public VulkanPipeline
{
public:
	ImGuiPipeline();
	virtual ~ImGuiPipeline();

public:
	void CreateRenderPass() override;

	void Rebuild() override;

	void CreatePipeline() override;

	void Update(const Scene* scene) override;

	void Render(const Scene* scene, const CommandBuffer& commandBuffer, uint32_t surfaceId) override;

private:
	void SetTheme();
	void DestroyFrameBuffers();

private:
	// the current ImGuiContext
	ImGuiContext*							m_Context;
	bool									m_BlockEvents = true;

	VkDescriptorPool						m_DescriptorPool = VK_NULL_HANDLE;
	VkRenderPass							m_Renderpass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer>				m_Framebuffers;
};

