#include "Scene.hpp"

#include "SceneSerializer.hpp"
#include "renderer/Camera.hpp"
#include "core/Time.hpp"

#include <iostream>

Scene::Scene()
{
	UpdateWorldUniform();
}

Scene::Scene(std::filesystem::path& path)
{
}

Scene::~Scene()
{
	Unload();
}

void Scene::Unload()
{

}

void Scene::Update()
{
	// TODO: update lightings
	//UpdateWorldUniform();

	// update camera
	Camera* cam = mainCam();
	if (cam == nullptr)
	{
		std::cerr << "No camera active in scene!\n";
		return;
	}
	static float time = 0;
	time += Time::DeltaTime;

	static Transform t(glm::vec3{ 0, 0, 20 }, glm::vec3(0, 0, 0));
	cam->Update(t);

	// update entities and components
	Entity::root().Update();

	// push all transforms to pipeline
	m_MatrixStack.Clear();
	m_ObjectInstances.clear();

	for (auto& child : Entity::root().children()) 
	{
		assert(m_MatrixStack.Size() == 0);
		child->RenderPass(m_MatrixStack);
 	}
}

void Scene::PushObjectInstances(const ObjectInstance&& instance)
{
	m_ObjectInstances.push_back(instance);
}

Camera* Scene::mainCam() const
{
	//if (m_Cameras.empty())
	//	return nullptr;

	//return m_Cameras.begin()->second;

	static std::unique_ptr<Camera> cam = std::make_unique<Camera>();
	return cam.get();
}

void Scene::UpdateWorldUniform()
{
	m_SceneInfo.SKY_DIRECTION.x = 0.0f;
	m_SceneInfo.SKY_DIRECTION.y = 0.0f;
	m_SceneInfo.SKY_DIRECTION.z = 1.0f;

	m_SceneInfo.SKY_ENERGY.r = 0.1f;
	m_SceneInfo.SKY_ENERGY.g = 0.1f;
	m_SceneInfo.SKY_ENERGY.b = 0.2f;

	m_SceneInfo.SUN_DIRECTION.x = 6.0f / 23.0f;
	m_SceneInfo.SUN_DIRECTION.y = 13.0f / 23.0f;
	m_SceneInfo.SUN_DIRECTION.z = -18.0f / 23.0f;

	m_SceneInfo.SUN_ENERGY.r = 1.0f;
	m_SceneInfo.SUN_ENERGY.g = 0.0f;
	m_SceneInfo.SUN_ENERGY.b = 0.0f;
}