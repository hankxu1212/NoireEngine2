#include "Scene.hpp"

#include "SceneSerializer.hpp"
#include "renderer/Camera.hpp"
#include "renderer/components/Components.hpp"
#include "core/Time.hpp"
#include "Entity.hpp"

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
	if (mainCam() == nullptr)
	{
		std::cerr << "Did not find main camera!\n";
		return;
	}

	// TODO: update lightings
	//UpdateWorldUniform();

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

CameraComponent* Scene::mainCam() const
{
	if (m_Cameras.empty())
		return nullptr;

	return m_Cameras.begin()->second;
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