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
		scene = std::make_unique<Scene>("../scenes/examples/sg-Articulation.s72");

	glm::quat q{ 1,0,0,0 };
	glm::vec3 s{ 1,1,1 };
	glm::vec3 e1T{ 0,0,20 };
	const std::string cameraName = "--Core::Debug Camera";
	Entity* e1 = scene->Instantiate(cameraName, e1T, q, s);
	e1->AddComponent<Core::SceneNavigationCamera>();
	e1->AddComponent<CameraComponent>(Camera::Type::Debug);

	// setup debug camera and camera script
	scene->m_DebugCamera = e1->GetComponent<CameraComponent>();
	scene->m_UserNavigationCamera = e1->GetComponent<Core::SceneNavigationCamera>();

	const std::string inputName = "--Core::Input";
	Entity* e2 = scene->Instantiate(inputName);
	e2->AddComponent<Core::Input>();
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
