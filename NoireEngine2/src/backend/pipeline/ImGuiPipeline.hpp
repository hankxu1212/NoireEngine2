#pragma once

#include "VulkanPipeline.hpp"
#include "backend/renderpass/Renderpass.hpp"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/backends/imgui_impl_glfw.h"

class RaytracingPipeline;
class Image2D;

class ImGuiPipeline : public VulkanPipeline
{
public:
	ImGuiPipeline();
	virtual ~ImGuiPipeline();

public:
	void CreateRenderPass() override;

	void CreatePipeline() override;

	void Rebuild() override;

	void Update(const Scene* scene) override;

	void Render(const Scene* scene, const CommandBuffer& commandBuffer) override;

	void AppendDebugImage(Image* image, const std::string& name);

	void AppendDebugImage(VkSampler sampler, VkImageView view, VkImageLayout layout, const std::string& name);

private:
	void DestroyFrameBuffers();

	void SetTheme();

private:
	// the current ImGuiContext
	ImGuiContext*							m_Context;
	bool									m_BlockEvents = true;

	VkDescriptorPool						m_DescriptorPool = VK_NULL_HANDLE;
	std::unique_ptr<Renderpass>				s_Renderpass;
	std::vector<VkFramebuffer>				m_FrameBuffers;

	std::vector<std::pair<VkDescriptorSet, std::string>> m_DebugImages;
};

