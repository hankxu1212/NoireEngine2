#pragma once

#include <iostream>
#include <vector>
#include <format>
#include <memory>

#include <vulkan/vulkan.h>

#include "core/Core.hpp"

#include "core/resources/Module.hpp"
#include "core/window/Window.hpp"

#include "devices/VulkanInstance.hpp"
#include "devices/PhysicalDevice.hpp"
#include "devices/LogicalDevice.hpp"
#include "devices/Surface.hpp"

#include "commands/CommandBuffer.hpp"
#include "commands/CommandPool.hpp"

class VulkanContext : public Module::Registrar<VulkanContext>
{
	inline static const bool Registered = Register(UpdateStage::Render, DestroyStage::Post, Requires<Window>());
public:
	VulkanContext();

	~VulkanContext();

	void Update();

	void OnWindowResize(uint32_t width, uint32_t height);

	void OnAddWindow(Window* window);

	void WaitForCommands();

public:
	static void VK_CHECK(VkResult err, const char* msg);

	static inline const VkDevice			GetDevice() { return *(VulkanContext::Get()->getLogicalDevice()); }
	inline const LogicalDevice*				getLogicalDevice() const { return s_LogicalDevice.get(); }
	inline const VulkanInstance*			getInstance() const { return s_Instance.get(); }
	inline const PhysicalDevice*			getPhysicalDevice() const { return s_PhysicalDevice.get(); }

	std::shared_ptr<CommandPool>&			GetCommandPool(const TID& threadId = std::this_thread::get_id());

private:
	std::unique_ptr<VulkanInstance>				s_Instance;
	std::unique_ptr<PhysicalDevice>				s_PhysicalDevice;
	std::unique_ptr<LogicalDevice>				s_LogicalDevice;

	struct PerSurfaceBuffers 
	{
		std::vector<VkSemaphore>	presentCompletesSemaphores;
		std::vector<VkSemaphore>	renderCompletesSemaphores;
		std::vector<VkFence>		flightFences;
		std::size_t					currentFrame = 0;
		bool						framebufferResized = false;

		std::vector<std::unique_ptr<CommandBuffer>>	commandBuffers;

		// present finished semaphore for current frame
		inline VkSemaphore getPresentSemaphore() { return presentCompletesSemaphores[currentFrame]; }
		// render finished semaphore for current frame
		inline VkSemaphore getRenderSemaphore() { return renderCompletesSemaphores[currentFrame]; }
		// cpu fence for current frame
		inline VkFence getFence() { return flightFences[currentFrame]; }
	};

	VkPipelineCache								m_PipelineCache = VK_NULL_HANDLE;

	std::unordered_map<TID, std::shared_ptr<CommandPool>>		m_CommandPools;

private:
	void CreatePipelineCache();
};
