#pragma once

#include <string>
#include <cassert>
#include <functional>
#include <mutex>
#include <map>

#include "utils/Singleton.hpp"
#include "resources/Module.hpp"
#include "core/events/ApplicationEvent.hpp"

int main(int argc, char** argv);

struct ApplicationCommandLineArgs
{
	int Count = 0;
	char** Args = nullptr;

	const char* operator[](int index) const
	{
		assert(index < Count);
		return Args[index];
	}
};

struct ApplicationSpecification
{
	std::string Name = "Noire Engine Application";
	std::string WorkingDirectory;
	uint32_t width, height;
	ApplicationCommandLineArgs CommandLineArgs;
};

class Application : Singleton
{
public:
	Application(const ApplicationSpecification& specification);
	~Application();

	static Application& Get() { return *s_Instance; }
	static const char* GetArgs() { return s_Instance->GetSpecification().CommandLineArgs.Args[0]; }
	static const ApplicationSpecification& GetSpecification() { return s_Instance->m_Specification; }
	static const uint16_t GetFPS() { return s_Instance->m_FPS; }

	void SubmitToMainThread(const std::function<void()>& function);
	void Close();

private:
	void Run();

	void ExecuteMainThreadQueue();

	bool OnWindowClose(WindowCloseEvent& e);
	bool OnWindowResize(WindowResizeEvent& e);

	void CreateModule(Module::RegistryMap::const_iterator it);
	void DestroyModule(TypeId id, Module::DestroyStage stage);
	void UpdateStage(Module::UpdateStage stage);
	void DestroyStage(Module::DestroyStage stage);

private:
	ApplicationSpecification			m_Specification;
	bool								m_Running = true;
	bool								m_Minimized = false;
	float								m_LastFrameTime = 0.0f;
	uint16_t							m_FPS;
	uint16_t							m_FPS_Accumulator;

	std::vector<std::function<void()>>	m_MainThreadQueue;
	std::mutex							m_MainThreadQueueMutex;

	std::map<TypeId, std::unique_ptr<Module>>				m_Modules;
	std::map<Module::UpdateStage, std::vector<TypeId>>			m_ModuleStages;
	std::map<Module::DestroyStage, std::vector<TypeId>>		m_ModuleDestroyStages;
private:
	static Application* s_Instance;
	friend int ::main(int argc, char** argv);
};