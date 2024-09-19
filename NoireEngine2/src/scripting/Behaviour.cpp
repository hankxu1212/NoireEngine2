#include "Behaviour.hpp"

#include "renderer/scene/Scene.hpp"

template<> 
void Scene::OnComponentAdded<Behaviour>(Entity& entity, Behaviour& component)
{
	component.Awake();
}

template<>
void Scene::OnComponentRemoved<Behaviour>(Entity& entity, Behaviour& component)
{
	component.Shutdown();
	ScriptingEngine::Get()->Remove(component.getClassName());
}