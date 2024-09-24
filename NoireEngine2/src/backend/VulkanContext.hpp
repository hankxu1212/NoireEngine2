#pragma once

#include <iostream>
#include <vector>
#include <format>
#include <memory>
#include <array>

#include <vulkan/vulkan.h>

#include "core/Core.hpp"
#include "core/window/Window.hpp"

#include "devices/VulkanInstance.hpp"
#include "devices/PhysicalDevice.hpp"
#include "devices/LogicalDevice.hpp"
#include "devices/Surface.hpp"

#include "commands/CommandBuffer.hpp"
#include "commands/CommandPool.hpp"

#include "images/ImageDepth.hpp"
#include "renderpass/Swapchain.hpp"

#include "renderer/Renderer.hpp"

/**
 * Manages Instance, Physical/Logical devices, Swapchains (to a certain extent) and surfaces.
 */
class VulkanContext : public Module::Registrar<VulkanContext>
{
	inline static const bool Registered = Register(UpdateStage::Render, DestroyStage::Post, Requires<Window>());

public:
	VulkanContext();

	virtual ~VulkanContext();

	void Update();

	void OnWindowResize(uint32_t width, uint32_t height);

	void OnAddWindow(Window* window);

	void InitializeRenderer();

	void WaitForCommands();

	/* Finds physical device memory properties given a certain type filter */
	static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	/**
	  * Find a format in the candidates list that fits the tiling and features required.
	  * @param candidates Formats that are tested for features, in order of preference.
	  * @param tiling Tiling mode to test features in.
	  * @param features The features to test for.
	  * @return The format found, or VK_FORMAT_UNDEFINED.
	*/
	static VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);


public:
	static void VK_CHECK(VkResult err, const char* msg);

	static inline const VkDevice			GetDevice() { return *(VulkanContext::Get()->getLogicalDevice()); }
	inline const LogicalDevice*				getLogicalDevice() const { return s_LogicalDevice.get(); }
	inline const VulkanInstance*			getInstance() const { return s_VulkanInstance.get(); }
	inline const PhysicalDevice*			getPhysicalDevice() const { return s_PhysicalDevice.get(); }
	inline const VkPipelineCache&			getPipelineCache() const { return m_PipelineCache; }

	std::shared_ptr<CommandPool>&			GetCommandPool(const TID& threadId = std::this_thread::get_id());

	inline const Surface*					getSurface(std::size_t id=0) const { return m_Surfaces[id].get(); }
	inline const SwapChain*					getSwapChain(std::size_t id=0) { return m_Swapchains[id].get(); }

	inline const VkSemaphore				getPresentSemaphore() { return m_PerSurfaceBuffers[0]->getPresentSemaphore(); }
	inline const VkSemaphore				getRenderSemaphore() { return m_PerSurfaceBuffers[0]->getRenderSemaphore(); }
	inline const VkFence					getFence() { return m_PerSurfaceBuffers[0]->getFence(); }
	inline const std::size_t				getCurrentFrame() { return m_PerSurfaceBuffers[0]->currentFrame; }
	inline const std::uint32_t				getWorkspaceSize() const { return static_cast<uint32_t>(m_PerSurfaceBuffers.size()); }

private:
	std::unique_ptr<VulkanInstance>				s_VulkanInstance;
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

	VkPipelineCache												m_PipelineCache = VK_NULL_HANDLE;

	std::unordered_map<TID, std::shared_ptr<CommandPool>>		m_CommandPools;

	std::vector<std::unique_ptr<PerSurfaceBuffers>>				m_PerSurfaceBuffers;
	std::vector<std::unique_ptr<Surface>>						m_Surfaces;
	std::vector<std::unique_ptr<SwapChain>>						m_Swapchains;

	std::unique_ptr<Renderer>									s_Renderer;

private:
	void CreatePipelineCache();
	void RecreateSwapchain();
};
