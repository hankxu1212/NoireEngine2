#include "Scene.hpp"

#include "SceneSerializer.hpp"
#include "renderer/Camera.hpp"

Scene::Scene()
{

}

Scene::Scene(std::filesystem::path& path)
{
}

void Scene::Unload()
{

}

void Scene::Update()
{
	Entity::root().Update();

	m_MatrixStack.Clear();
	Entity::root().RenderPass(m_MatrixStack);
}