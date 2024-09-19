#include "CameraComponent.hpp"

#include "renderer/scene/Scene.hpp"
#include <iostream>

void CameraComponent::Update()
{
	s_Camera->Update(*GetTransform());
}

CameraComponent::CameraComponent()
{
	std::cout << "Created camera component\n";
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