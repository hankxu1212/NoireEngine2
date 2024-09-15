#pragma once

#include <vulkan/vulkan.h>

#include "core/resources/Module.hpp"
#include "core/window/Window.hpp"

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
	/* Checks validity of `err`*/
	static void VK_CHECK(VkResult err, const char* msg);
};

