#pragma once

#include "core/Application.h"
#include "backend/vulkan/renderer/VulkanRenderModule.h"
#include "backend/vulkan/pipeline/VulkanGraphicsPipeline.h"

namespace Noire {
	class ImGuiRenderer : public VulkanRenderModule
	{
	public:
		explicit ImGuiRenderer(const VulkanPipeline::Stage& pipelineStage) : 
			VulkanRenderModule(pipelineStage) {
		}

		void Render(const VulkanCommandBuffer& commandBuffer) override {
			auto &app = Application::Get();
			auto p_ImGguiLayer = app.GetImGuiLayer();

			p_ImGguiLayer->Begin();
			{
				for (Layer* layer : app.getLayerStack())
					layer->OnImGuiRender();
			}
			p_ImGguiLayer->End();

			p_ImGguiLayer->GraphicsRender(commandBuffer);
		}

		void Rebuild(const VulkanRenderStage& renderStage) override {
			Application::Get().GetImGuiLayer()->InitializeImGuiVulkan(renderStage.getRenderpass());
		}
	};

}

