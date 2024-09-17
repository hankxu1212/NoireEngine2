#pragma once

#include <string>
#include <cassert>
#include <functional>
#include <mutex>
#include <map>

#include "utils/Singleton.hpp"
#include "core/events/ApplicationEvent.hpp"
#include "core/layers/LayerStack.hpp"

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
	static Application& Get() { return *s_Instance; }
	static Application* s_Instance;

public:
	Application(const ApplicationSpecification& specification);
	~Application();

	void SubmitToMainThread(const std::function<void()>& function);

	void OnEvent(Event& e);
	
	void PushLayer(Layer* layer);
	void PushOverlay(Layer* layer);

	void Close();

	static const char* GetArgs() { return s_Instance->GetSpecification().CommandLineArgs.Args[0]; }
	static const ApplicationSpecification& GetSpecification() { return s_Instance->m_Specification; }
	static const uint16_t GetFPS() { return s_Instance->m_FPS; }
	static const LayerStack& GetLayerStack() { return s_Instance->m_LayerStack; }

private:
	void Run();

	void ExecuteMainThreadQueue();

	bool OnWindowClose(WindowCloseEvent& e);
	bool OnWindowResize(WindowResizeEvent& e);

private:
	ApplicationSpecification			m_Specification;
	bool								m_Running = true;
	bool								m_Minimized = false;
	float								m_LastFrameTime = 0.0f;
	uint16_t							m_FPS = 0;
	uint16_t							m_FPS_Accumulator = 0;

	LayerStack							m_LayerStack;

	std::vector<std::function<void()>>	m_MainThreadQueue;
	std::mutex							m_MainThreadQueueMutex;

private:
	friend int ::main(int argc, char** argv);
};