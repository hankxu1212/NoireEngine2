#include <chrono>

#include "core/Core.hpp"
#include "Application.hpp"
#include "core/Time.hpp"
#include "core/window/Window.hpp"

Application* Application::s_Instance = nullptr;
float Time::DeltaTime;

Application::Application(const ApplicationSpecification& specification)
	: m_Specification(specification)
{
	s_Instance = this;

	for (auto it = Module::GetRegistry().begin(); it != Module::GetRegistry().end(); ++it)
		CreateModule(it);

	// initializes window
	Window::Get()->SetEventCallback(NE_BIND_EVENT_FN(Application::OnEvent));
}

Application::~Application()
{
	DestroyStage(Module::DestroyStage::Pre);

	m_LayerStack.Detach();

	DestroyStage(Module::DestroyStage::Normal);

	m_LayerStack.Destroy();

	DestroyStage(Module::DestroyStage::Post);
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

		UpdateStage(Module::UpdateStage::Always);

		if (!m_Minimized)
		{
			UpdateStage(Module::UpdateStage::Pre);

			UpdateStage(Module::UpdateStage::Normal);

			for (Layer* layer : m_LayerStack)
			{
				layer->OnUpdate();
			}

			UpdateStage(Module::UpdateStage::Post);

			UpdateStage(Module::UpdateStage::Render);
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
	std::cout << e << std::endl;

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

	//Renderer::OnWindowResize(e.getWidth(), e.getHeight());
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

void Application::CreateModule(Module::RegistryMap::const_iterator it) 
{
	if (m_Modules.find(it->first) != m_Modules.end())
		return;

	// TODO: Prevent circular dependencies.
	for (auto requireId : it->second.requiredModules)
		CreateModule(Module::GetRegistry().find(requireId));

	auto&& module = it->second.create();
	m_Modules[it->first] = std::move(module);
	m_ModuleStages[it->second.stage].emplace_back(it->first);
	m_ModuleDestroyStages[it->second.destroyStage].emplace_back(it->first);
}

void Application::DestroyModule(TypeId id, Module::DestroyStage stage) 
{
	if (!m_Modules[id])
		return;

	// Destroy all module dependencies first.
	for (const auto& [registrarId, registrar] : Module::GetRegistry()) {
		if (std::find(registrar.requiredModules.begin(), registrar.requiredModules.end(), id) != registrar.requiredModules.end()
			&& registrar.destroyStage == stage) {
			DestroyModule(registrarId, stage);
		}
	}

	// TODO: check if a module has been deleted
	std::cout << "Trying to delete module ID: " << id << std::endl;
	m_Modules[id].reset();
	std::cout << "Successfully deleted module ID: " << id << std::endl;
}

void Application::UpdateStage(Module::UpdateStage stage) 
{
	for (auto& moduleId : m_ModuleStages[stage])
		m_Modules[moduleId]->Update();
}

void Application::DestroyStage(Module::DestroyStage stage)
{
	for (auto& moduleId : m_ModuleDestroyStages[stage])
		DestroyModule(moduleId, stage);
}