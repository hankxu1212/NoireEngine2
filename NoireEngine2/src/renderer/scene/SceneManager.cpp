#include "SceneManager.hpp"
#include "renderer/scene/Entity.hpp"
#include "renderer/components/Components.hpp"
#include "renderer/Camera.hpp"
#include "Application.hpp"

//A:\Blender\blender --background --python export - s72.py-- "F:\gameDev\Engines\NoireEngine2\NoireEngine2\src\scenes\examples\sources\manygameobjects.blend" "F:\gameDev\Engines\NoireEngine2\NoireEngine2\src\scenes\examples\manygameobjects.s72" --collection Collection

SceneManager::SceneManager()
{
	if (Application::Get().GetSpecification().InitialScene)
		scene = std::make_unique<Scene>(Application::GetSpecification().InitialScene.value());
	else
		scene = std::make_unique<Scene>("../scenes/examples/lights-Mix.s72");
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
