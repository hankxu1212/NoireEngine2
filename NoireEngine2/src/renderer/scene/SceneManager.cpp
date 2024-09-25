#include "SceneManager.hpp"
#include "renderer/scene/Entity.hpp"
#include "renderer/components/Components.hpp"
#include "Application.hpp"

SceneManager::SceneManager()
{
	if (Application::Get().GetSpecification().InitialScene)
		scene = std::make_unique<Scene>(Application::Get().GetSpecification().InitialScene.value());
	else
		scene = std::make_unique<Scene>("../scenes/examples/sg-Articulation.s72");

	glm::quat q{ 1,0,0,0 };
	glm::vec3 s{ 1,1,1 };

	glm::vec3 e1T{ 0,0,20 };
	const std::string cameraName = "Main Camera";
	Entity* e1 = scene->Instantiate(cameraName, e1T, q, s);
	e1->AddComponent<CameraComponent>(-100);
	e1->AddComponent<Core::SceneNavigationCamera>();

	const std::string inputName = "Input";
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
