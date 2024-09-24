#pragma once

#include "core/layers/Layer.hpp"
#include "core/events/ApplicationEvent.hpp"
#include "core/events/KeyEvent.hpp"
#include "core/events/MouseEvent.hpp"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/backends/imgui_impl_glfw.h"

#include "vulkan/vulkan.h"
#include <glm/glm.hpp>

class CommandBuffer;
class SwapChain;

class ImGuiLayer : public Layer
{
public:
	ImGuiLayer();
	~ImGuiLayer();

	void OnAttach() override;
	void OnEvent(Event& e) override;

	void Begin();
	void End();

	void RenderViewport();

	ImGuiContext* getContext() const { return m_Context; }

	//uint32_t getActiveWidgetID() const { return GImGui->ActiveId; }

	inline const glm::vec4& getViewportBounds() { return m_ViewportBounds; }

	void BlockEvents(bool block) { m_BlockEvents = block; }

private:
	void SetTheme();
	void CreateViewportDescriptorSets();

private:
	// the current ImGuiContext
	ImGuiContext*			m_Context;
	bool					m_BlockEvents = true;

	VkDescriptorPool		m_DescriptorPool = VK_NULL_HANDLE;
	VkRenderPass			m_Renderpass = VK_NULL_HANDLE;

	std::vector<VkDescriptorSet>	m_ViewportDescriptors;

	glm::vec4 m_ViewportBounds;
};
