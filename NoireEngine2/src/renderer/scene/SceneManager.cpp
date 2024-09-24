#include "SceneManager.hpp"
#include "renderer/scene/Entity.hpp"
#include "renderer/components/Components.hpp"

SceneManager::SceneManager()
{
	scene = std::make_unique<Scene>("../scenes/examples/sg-Articulation.s72");

	glm::quat q{ 1,0,0,0 };
	glm::vec3 s{ 1,1,1 };

	glm::vec3 e1T{ 0,0,20 };
	Entity* e1 = scene->Instantiate(e1T, q, s);
	e1->AddComponent<CameraComponent>(-100);
	e1->AddComponent<Core::SceneNavigationCamera>();

	Entity* e2 = scene->Instantiate();
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
