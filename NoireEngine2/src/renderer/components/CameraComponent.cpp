#include "CameraComponent.hpp"

#include "renderer/scene/Scene.hpp"

void CameraComponent::Update()
{
	s_Camera->Update(*GetTransform());
}

CameraComponent::CameraComponent()
{
	s_Camera = std::make_unique<Camera>();
}

template<>
void Scene::OnComponentAdded<CameraComponent>(Entity& entity, CameraComponent& component)
{
	m_Cameras.insert(component.makeKey());
}

template<>
void Scene::OnComponentRemoved<CameraComponent>(Entity& entity, CameraComponent& component)
{
	m_Cameras.erase(m_Cameras.find(component.makeKey()));
}