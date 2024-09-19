#include <chrono>

#include "core/Core.hpp"
#include "Application.hpp"
#include "core/Time.hpp"
#include "core/window/Window.hpp"
#include "backend/VulkanContext.hpp"

#include "scripting/ScriptingEngine.hpp"

Application* Application::s_Instance = nullptr;
float Time::DeltaTime;

Application::Application(const ApplicationSpecification& specification)
	: m_Specification(specification)
{
	s_Instance = this;

	Window::Initialize();
	VulkanContext::Initialize();

	// initializes window
	Window::Get().SetEventCallback(NE_BIND_EVENT_FN(Application::OnEvent));
	VulkanContext::Get().OnAddWindow(&Window::Get());

	PushLayer(new ScriptingEngine());
}

Application::~Application()
{
	m_LayerStack.Detach();
	m_LayerStack.Destroy();

	VulkanContext::Destroy();
	Window::Destroy();
}

void Application::Run()
{
	auto before = std::chrono::high_resolution_clock::now();
	while (m_Running)
	{
		auto after = std::chrono::high_resolution_clock::now();
		float dt = float(std::chrono::duration<double>(after - before).count());
		before = after;

		Time::DeltaTime = std::min(dt, 0.1f); //lag if frame rate dips too low

		ExecuteMainThreadQueue();

		if (!m_Minimized)
		{
			Window::Get().Update();

			for (Layer* layer : m_LayerStack)
			{
				layer->OnUpdate();
			}
 			VulkanContext::Get().Update();
		}
	}
}

void Application::ExecuteMainThreadQueue()
{
	if (m_MainThreadQueue.empty())
		return;

	std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);

	for (auto& func : m_MainThreadQueue)
		func();

	m_MainThreadQueue.clear();
}

void Application::SubmitToMainThread(const std::function<void()>& function)
{
	std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);

	m_MainThreadQueue.emplace_back(function);
}

void Application::OnEvent(Event& e)
{
	e.Dispatch<WindowCloseEvent>(NE_BIND_EVENT_FN(Application::OnWindowClose));
	e.Dispatch<WindowResizeEvent>(NE_BIND_EVENT_FN(Application::OnWindowResize));

	for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
	{
		if (e.Handled)
			break;
		(*it)->OnEvent(e);
	}
}

bool Application::OnWindowClose(WindowCloseEvent& e)
{
	m_Running = false;
	return true;
}

bool Application::OnWindowResize(WindowResizeEvent& e)
{
	if (e.m_Width == 0 || e.m_Height == 0)
	{
		m_Minimized = true;
		return false;
	}

	VulkanContext::Get().OnWindowResize(e.m_Width, e.m_Height);
	m_Minimized = false;
	return false;
}

void Application::PushLayer(Layer* layer)
{
	m_LayerStack.PushLayer(layer);
	layer->OnAttach();
}

void Application::PushOverlay(Layer* layer)
{
	m_LayerStack.PushOverlay(layer);
	layer->OnAttach();
}

void Application::Close()
{
	m_Running = false;
}