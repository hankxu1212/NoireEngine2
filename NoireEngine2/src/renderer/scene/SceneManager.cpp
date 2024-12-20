#include "SceneManager.hpp"
#include "renderer/scene/Entity.hpp"
#include "renderer/components/Components.hpp"
#include "renderer/Camera.hpp"
#include "Application.hpp"

//A:\Blender\blender --background --python export-s72.py -- "F:\gameDev\Engines\NoireEngine2\NoireEngine2\src\scenes\LightsScene\LightsScene.blend" "F:\gameDev\Engines\NoireEngine2\NoireEngine2\src\scenes\LightsScene\LightsScene.s72" --collection Collection

SceneManager::SceneManager()
{
	scene = std::make_unique<Scene>();
}

SceneManager::~SceneManager()
{
	Shutdown();
}

void SceneManager::LoadDefault()
{
	if (Application::Get().GetSpecification().InitialScene)
		scene->Load(Application::GetSpecification().InitialScene.value());
	else
		//scene->Load("../scenes/examples/sphereflake.s72");
		//scene->Load("../scenes/SphereScene/SphereScene.s72");
		//scene->Load("../scenes/examples/materials.s72");
		//scene->Load("../scenes/BrickScene/BrickScene.s72");
		//scene->Load("../scenes/examples/lights-Mix.s72");
		//scene->Load("../scenes/examples/lights-Parameters.s72");
		//scene->Load("../scenes/examples/lights-Spot-Shadows.s72");
		//scene->Load("../scenes/LightsScene/LightsScene.s72");
		//scene->Load("../scenes/ManyLights/ManyLights.s72");
		//scene->Load("../scenes/LightsLimitTest/LightsLimit1247.s72");
		scene->Load("../scenes/Reflections/Reflections.s72");

	SetCameraMode(Scene::CameraMode::User);
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
	scene->m_UserNavigationCamera->enabled = cameraMode != Scene::CameraMode::Scene;
}
