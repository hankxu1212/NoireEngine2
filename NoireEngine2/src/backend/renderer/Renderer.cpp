#include "Renderer.hpp"

#include <memory>

#include "backend/VulkanContext.hpp"
#include "renderer/scene/Entity.hpp"
#include "renderer/scene/Scene.hpp"
#include "renderer/components/Components.hpp"

Renderer::Renderer()
{
	scene = new Scene("../scenes/examples/sg-Articulation.s72");

	glm::quat q{ 1,0,0,0 };
	glm::vec3 s{ 1,1,1 };

	glm::vec3 e1T{ 0,0,20 };
	Entity* e1 = scene->Instantiate(e1T, q, s);
	e1->AddComponent<CameraComponent>(-100);
	e1->AddComponent<Core::SceneNavigationCamera>();

	Entity* e2 = scene->Instantiate();
	e2->AddComponent<Core::Input>();

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
	delete scene;
}

void Renderer::Update()
{
	scene->Update();
	objectPipeline->Update(scene);
	imguiPipeline->Update(scene);
}

void Renderer::Render(const CommandBuffer& commandBuffer, uint32_t surfaceId)
{
	objectPipeline->Render(scene, commandBuffer, surfaceId);
	imguiPipeline->Render(scene, commandBuffer, surfaceId);
}

void Renderer::Rebuild()
{
	objectPipeline->Rebuild();
	imguiPipeline->Rebuild();
}