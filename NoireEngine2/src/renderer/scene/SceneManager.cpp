#include "SceneManager.hpp"
#include "renderer/scene/Entity.hpp"
#include "renderer/components/Components.hpp"
#include "renderer/Camera.hpp"
#include "Application.hpp"

SceneManager::SceneManager()
{
	if (Application::Get().GetSpecification().InitialScene)
		scene = std::make_unique<Scene>(Application::GetSpecification().InitialScene.value());
	else
		scene = std::make_unique<Scene>("../scenes/examples/rotation.s72");
}

SceneManager::~SceneManager()
{
	Shutdown();
}

void SceneManager::Update()
{
	if (!scene)
		return;

	scene->Update();
	scene->Render();
}

void SceneManager::Shutdown()
{
	if (scene) {
		scene->Unload();
		scene.reset();
	}
}

void SceneManager::SetCameraMode(Scene::CameraMode newMode)
{
	cameraMode = newMode;
	if (cameraMode == Scene::CameraMode::Scene)
		scene->m_UserNavigationCamera->enabled = false;
	else
		scene->m_UserNavigationCamera->enabled = true;
}
