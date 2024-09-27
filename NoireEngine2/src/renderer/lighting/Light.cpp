#include "Light.hpp"
#include "renderer/scene/Scene.hpp"

void Light::Update()
{
}

template<>
void Scene::OnComponentAdded<Light>(Entity& entity, Light& component)
{
}

template<>
void Scene::OnComponentRemoved<Light>(Entity& entity, Light& component)
{
}