#pragma once

#include "GLFW/glfw3.h"

#include "Application.hpp"

// Represents an abstraction on GLFW window
class Window : public Module::Registrar<Window>
{
	inline static const bool Registered = Register(UpdateStage::Pre, DestroyStage::Post);

public:
	Window();
	
	virtual ~Window();

	void Update();

	float inline getAspectRatio() const { return static_cast<float>(m_Data.Width) / m_Data.Height; }

	using EventCallbackFn = std::function<void(Event&)>;
	
	inline void SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }

public:
	struct WindowData
	{
		std::string Title;
		unsigned int Width, Height;
		bool VSync;

		EventCallbackFn EventCallback;
	}m_Data;

	GLFWwindow* m_Window;
};
