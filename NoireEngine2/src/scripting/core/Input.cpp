#include "Input.hpp"

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
