#include "SceneManager.hpp"
#include "renderer/scene/Entity.hpp"
#include "renderer/components/Components.hpp"
#include "renderer/Camera.hpp"
#include "Application.hpp"

//A:\Blender\blender --background --python export - s72.py-- "F:\gameDev\Engines\NoireEngine2\NoireEngine2\src\scenes\examples\sources\manygameobjects.blend" "F:\gameDev\Engines\NoireEngine2\NoireEngine2\src\scenes\examples\manygameobjects.s72" --collection Collection

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
		scene->Load("../scenes/SphereScene/SphereScene.s72");
		//scene->Load("../scenes/examples/Materials.s72");
}

//"displacementMap": { "src": "Dirt.displacement.png" },
//"normalMap": { "src": "Dirt.normal.png" },

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
