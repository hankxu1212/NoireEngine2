#pragma once

#include <iostream>
#include <vector>
#include <format>
#include <memory>

#include <vulkan/vulkan.h>

#include "core/resources/Module.hpp"
#include "core/window/Window.hpp"

#include "devices/VulkanInstance.hpp"
#include "devices/PhysicalDevice.hpp"
#include "devices/LogicalDevice.hpp"
#include "devices/Surface.hpp"

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

	inline const VulkanInstance*			getInstance() const { return s_Instance.get(); }
	inline const PhysicalDevice*			getPhysicalDevice() const { return s_PhysicalDevice.get(); }

	static inline const VkDevice			GetDevice() { return *(VulkanContext::Get()->getLogicalDevice()); }
	inline const LogicalDevice*				getLogicalDevice() const { return s_LogicalDevice.get(); }

private:
	std::unique_ptr<VulkanInstance>				s_Instance;
	std::unique_ptr<PhysicalDevice>				s_PhysicalDevice;
	std::unique_ptr<LogicalDevice>				s_LogicalDevice;

	VkPipelineCache					m_PipelineCache = VK_NULL_HANDLE;
private:
	void CreatePipelineCache();
};
