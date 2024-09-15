#include <chrono>

#include "core/Core.hpp"
#include "Application.hpp"
#include "core/Time.hpp"

float Time::DeltaTime;

Application::Application(const ApplicationSpecification& specification)
{
}

Application::~Application()
{
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