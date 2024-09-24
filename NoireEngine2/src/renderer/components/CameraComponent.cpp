#include "CameraComponent.hpp"

#include "renderer/scene/Scene.hpp"
#include "utils/Logger.hpp"
#include <iostream>

void CameraComponent::Update()
{
	s_Camera->Update(*GetTransform());
}

CameraComponent::CameraComponent(int priority_) :
	priority(priority_)
{
	s_Camera = std::make_unique<Camera>();
}

template<>
void Scene::OnComponentAdded<CameraComponent>(Entity& entity, CameraComponent& component)
{
	NE_DEBUG(std::format("Creating new camera instance with priority {}", component.priority), Logger::YELLOW, Logger::BOLD);
	m_Cameras.insert(component.makeKey());
}

template<>
void Scene::OnComponentRemoved<CameraComponent>(Entity& entity, CameraComponent& component)
{
	NE_DEBUG(std::format("Removing camera instance with priority {}", component.priority), Logger::YELLOW);
	m_Cameras.erase(m_Cameras.find(component.makeKey()));
}