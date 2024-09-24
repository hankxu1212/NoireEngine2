#include "Renderer.hpp"

#include <memory>
#include "renderer/scene/SceneManager.hpp"
#include "backend/VulkanContext.hpp"

Renderer::Renderer()
{
	// initialize pipelines
	objectPipeline = std::make_unique<ObjectPipeline>();
	imguiPipeline = std::make_unique<ImGuiPipeline>();
}

void Renderer::CreateRenderPass()
{
	objectPipeline->CreateRenderPass();
	imguiPipeline->CreateRenderPass();
}

void Renderer::CreatePipelines()
{
	objectPipeline->CreatePipeline();
	imguiPipeline->CreatePipeline();
}

void Renderer::Cleanup()
{
	objectPipeline.reset();
	imguiPipeline.reset();
}

void Renderer::Update()
{
	objectPipeline->Update(SceneManager::Get()->getScene());
	imguiPipeline->Update(SceneManager::Get()->getScene());
}

void Renderer::Render(const CommandBuffer& commandBuffer, uint32_t surfaceId)
{
	objectPipeline->Render(SceneManager::Get()->getScene(), commandBuffer, surfaceId);
	imguiPipeline->Render(SceneManager::Get()->getScene(), commandBuffer, surfaceId);
}

void Renderer::Rebuild()
{
	objectPipeline->Rebuild();
	imguiPipeline->Rebuild();
}