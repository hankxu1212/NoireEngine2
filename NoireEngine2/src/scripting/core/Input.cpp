#include "Input.hpp"
#include "renderer/scene/Scene.hpp"

namespace Core
{
	Input* Input::g_Instance = nullptr;

	Input::Input()
	{
		g_Instance = this;
	}

	Input* Input::Get() 
	{
		return g_Instance;
	}

	void Input::Start()
	{
		lastMouseScreenPos = NativeInput::GetMousePosition();
	}

	void Input::Update()
	{
		lastMouseScreenPos = NativeInput::GetMousePosition();
	}

	glm::vec2 Input::GetMouseDelta() const
	{
		return NativeInput::GetMousePosition() - lastMouseScreenPos;
	}
}

template<>
void Scene::OnComponentAdded<Core::Input>(Entity& entity, Core::Input& component)
{
	this->OnComponentAdded<Behaviour>(entity, component);
}

template<>
void Scene::OnComponentRemoved<Core::Input>(Entity& entity, Core::Input& component)
{
	this->OnComponentRemoved<Behaviour>(entity, component);
}