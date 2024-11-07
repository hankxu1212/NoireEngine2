#pragma once

#include <iostream>
#include <vector>
#include <format>
#include <memory>
#include <array>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include "core/Core.hpp"
#include "core/window/Window.hpp"
#include "utils/Logger.hpp"
#include "core/Timer.hpp"
#include "core/resources/Files.hpp"

#include "devices/VulkanInstance.hpp"
#include "devices/PhysicalDevice.hpp"
#include "devices/LogicalDevice.hpp"
#include "devices/Surface.hpp"

#include "commands/CommandBuffer.hpp"
#include "commands/CommandPool.hpp"

#include "images/ImageDepth.hpp"
#include "renderpass/Swapchain.hpp"

#include "renderer/Renderer.hpp"

#include "backend/descriptor/DescriptorLayoutCache.hpp"
#include "core/resources/Resources.hpp"

/**
 * Manages Instance, Physical/Logical devices, Swapchains (to a certain extent) and surfaces.
 */
class VulkanContext : public Module::Registrar<VulkanContext>
{
	inline static const bool Registered = Register(
		UpdateStage::Render, 
		DestroyStage::Post, 
		Requires<Window, Resources>()
	);

public:
	VulkanContext();

	virtual ~VulkanContext();

	void LateInitialize() override;

	void Update();

	void OnWindowResize();

	void OnAddWindow(Window* window);

	void WaitGraphicsQueue();

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
	static void VK(VkResult err, const char* msg);
	static void VK(VkResult err);

	static inline const VkDevice			GetDevice() { return *(VulkanContext::Get()->getLogicalDevice()); }
	inline const LogicalDevice*				getLogicalDevice() const { return s_LogicalDevice.get(); }
	inline const VulkanInstance*			getInstance() const { return s_VulkanInstance.get(); }
	inline const PhysicalDevice*			getPhysicalDevice() const { return s_PhysicalDevice.get(); }
	inline const VkPipelineCache&			getPipelineCache() const { return m_PipelineCache; }

	std::shared_ptr<CommandPool>			GetCommandPool(const TID& threadId = std::this_thread::get_id());

	inline const Surface* getSurface(std::size_t id = 0) const
	{
		if (m_Surfaces.empty())
			return nullptr;
		return m_Surfaces[id].get();
	}

	inline const SwapChain* getSwapChain(std::size_t id = 0)
	{
		if (m_Swapchains.empty())
			return nullptr;
		return m_Swapchains[id].get();
	}

	inline const VkSemaphore				getPresentSemaphore() { return m_PerSurfaceBuffers[0]->getPresentSemaphore(); }
	inline const VkSemaphore				getRenderSemaphore() { return m_PerSurfaceBuffers[0]->getRenderSemaphore(); }
	inline const VkFence					getFence() { return m_PerSurfaceBuffers[0]->getFence(); }
	inline const size_t						getCurrentFrame() { return m_PerSurfaceBuffers[0]->currentFrame; }
	inline Buffer*							getIndirectBuffer() { return m_PerSurfaceBuffers[0]->getIndirectBuffer(); }
	inline const uint32_t					getFramesInFlight() const { return m_Swapchains[0]->getImageCount(); }

	inline DescriptorLayoutCache*			getDescriptorLayoutCache() { return &m_DescriptorLayoutCache; }

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

		std::vector<std::unique_ptr<CommandBuffer>>	commandBuffers;

		// present finished semaphore for current frame
		inline VkSemaphore getPresentSemaphore() { return presentCompletesSemaphores[currentFrame]; }
		// render finished semaphore for current frame
		inline VkSemaphore getRenderSemaphore() { return renderCompletesSemaphores[currentFrame]; }
		// cpu fence for current frame
		inline VkFence getFence() { return flightFences[currentFrame]; }

		inline Buffer* getIndirectBuffer() { return drawIndirectBuffers[currentFrame].get(); }

		// draw indirect buffers
		std::vector<std::unique_ptr<Buffer>>			drawIndirectBuffers;
	};

	VkPipelineCache												m_PipelineCache = VK_NULL_HANDLE;

	std::unordered_map<TID, std::shared_ptr<CommandPool>>		m_CommandPools;

	std::vector<std::unique_ptr<PerSurfaceBuffers>>				m_PerSurfaceBuffers;
	std::vector<std::unique_ptr<Surface>>						m_Surfaces;
	std::vector<std::unique_ptr<SwapChain>>						m_Swapchains;

	std::unique_ptr<Renderer>									s_Renderer;

	DescriptorLayoutCache										m_DescriptorLayoutCache;

private:
	void CreatePipelineCache();
	void RecreateSwapchain();
	void DestroyPerSurfaceStructs();
};

#define CURR_FRAME VulkanContext::Get()->getCurrentFrame()
