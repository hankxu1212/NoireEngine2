#include <chrono>

#include "core/Core.hpp"
#include "Application.hpp"
#include "core/Time.hpp"
#include "core/window/Window.hpp"
#include "backend/VulkanContext.hpp"

#include "scripting/ScriptingEngine.hpp"
#include "core/resources/Resources.hpp"
#include "editor/Editor.hpp"
#include "core/Timer.hpp"

#include "renderer/scene/SceneManager.hpp"

Application* Application::s_Instance = nullptr;

Application::Application(const ApplicationSpecification& specification)
	: m_Specification(specification)
{
	s_Instance = this;

	auto& registry = Module::GetRegistry();
	for (auto it = registry.begin(); it != registry.end(); ++it)
		CreateModule(it);

	// initialize glfw window
	
	// initialize instance, physical device, and logical device

	// the application will only serve as modules if alternativeApplication is true
	if (specification.alternativeApplication)
		return;

	// initialize scripting engine
	ScriptingEngine* scriptingEngine = new ScriptingEngine();

	// load default scene
	SceneManager::Get()->LoadDefault();

	// all objects pushed here

	// imgui context initialized here

	// bind window
	Window::Get()->SetEventCallback(NE_BIND_EVENT_FN(Application::OnEvent));

	// initialize new surface, create new render pass, frame bufferr, swapchain
	VulkanContext::Get()->OnAddWindow(Window::Get());

	// create pipelines here

	// create object pipeline, descriptor sets and pools and scene buffers

	// calls start on all scripts here
	PushLayer(scriptingEngine);

	// push editor window
	PushLayer(new Editor());
}

Application::~Application()
{
	VulkanContext::Get()->WaitIdle();

	DestroyStage(Module::DestroyStage::Pre);

	m_LayerStack.Detach();

	DestroyStage(Module::DestroyStage::Normal);

	m_LayerStack.Destroy();

	DestroyStage(Module::DestroyStage::Post);
}

void Application::Run()
{
	float lastSecondTime = (float)Time::GetTime();
	auto before = std::chrono::high_resolution_clock::now();

	while (m_Running)
	{
		auto after = std::chrono::high_resolution_clock::now();
		float dt = float(std::chrono::duration<double>(after - before).count());
		before = after;

		Time::DeltaTime = std::min(dt, 0.1f); //lag if frame rate dips too low
		Time::Now += Time::DeltaTime;

		m_FPS_Accumulator++;

		// displays fps information
		float currTime = (float)Time::GetTime();
		if (currTime - lastSecondTime >= 1.0)
		{
			m_FPS = m_FPS_Accumulator;
			lastSecondTime = currTime;
			m_FPS_Accumulator = 0;
			StatsDirty = true;
		}

		ExecuteMainThreadQueue();

		Timer timer;
		{
			RunUpdate();
		}
		if (StatsDirty)
			ApplicationUpdateTime = timer.GetElapsed(true);

		if (!m_Minimized) 
		{
			HandleWindowResizeComplete();

			RunRender();
			if (StatsDirty)
				ApplicationRenderTime = timer.GetElapsed(false);
		}
		StatsDirty = false;
	}
}

void Application::RunUpdate()
{
	UpdateStage(Module::UpdateStage::Pre);

	UpdateStage(Module::UpdateStage::Normal);

	for (Layer* layer : m_LayerStack)
	{
		layer->OnUpdate();
	}

	UpdateStage(Module::UpdateStage::Post);
}

void Application::RunRender()
{
	UpdateStage(Module::UpdateStage::Render);
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
	e.Dispatch<WindowIconfyEvent>(NE_BIND_EVENT_FN(Application::OnWindowIconfy));
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

bool Application::OnWindowIconfy(WindowIconfyEvent& e)
{
	std::cout << e.m_Minimized;
	m_Minimized = e.m_Minimized;
	return true;
}

bool Application::OnWindowResize(WindowResizeEvent& e)
{
	if (e.m_Width == 0 || e.m_Height == 0)
	{
		m_Minimized = true;
		return true;
	}

	isResizing = true;
	m_Minimized = false;
	return false;
}

void Application::HandleWindowResizeComplete()
{
	// only ever resize on release button
	if (isResizing && NativeInput::GetMouseButtonRelease(Mouse::ButtonLeft)) {
		VulkanContext::Get()->OnWindowResize();
		isResizing = false;
	}
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

	m_Modules[id].reset();
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