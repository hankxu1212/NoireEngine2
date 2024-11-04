#pragma once

#include <vulkan/vulkan.h>
#include <memory>

class CommandPool;

class CommandBuffer 
{
public:
	/**
	  * Creates a new command buffer.
	  * @param begin If wants to call Begin() right after initialization
	  * @param queueType The queue to run this command buffer on.
	  * @param bufferLevel The buffer level.
	*/
	explicit CommandBuffer(
		bool begin = true, 
		VkQueueFlagBits queueType = VK_QUEUE_GRAPHICS_BIT, 
		VkCommandBufferLevel bufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY
	);

	~CommandBuffer();

	/**
	  * Begins the recording state for this command buffer.
	  * @param usage How this command buffer will be used.
	*/
	void Begin(
		VkCommandBufferUsageFlags usage = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		const VkCommandBufferInheritanceInfo* inheritance = nullptr
	);

	// Ends the recording state for this command buffer.
	void End();

	// Submits the command buffer to the queue and will hold the current queue idle until it has finished.
	void SubmitIdle();

	// submit idle, and also wait on a fence
	void SubmitWait();

	/**
	  * Submits the command buffer.
	  * @param waitSemaphore A optional semaphore that will waited upon before the command buffer is executed.
	  * @param signalSemaphore A optional that is signaled once the command buffer has been executed.
	  * @param fence A optional fence that is signaled once the command buffer has completed.
	*/
	void Submit(
		const VkSemaphore& waitSemaphore = VK_NULL_HANDLE, 
		const VkSemaphore& signalSemaphore = VK_NULL_HANDLE, 
		VkFence fence = VK_NULL_HANDLE
	);

	operator const VkCommandBuffer& () const { return m_CommandBuffer; }
	const VkCommandBuffer& getCommandBuffer() const { return m_CommandBuffer; }

	bool IsRunning() const { return running; }

private:
	VkQueue GetQueue() const;

	std::shared_ptr<CommandPool>	r_CommandPool;

	VkQueueFlagBits					m_QueueType;
	VkCommandBuffer					m_CommandBuffer = VK_NULL_HANDLE;
	bool							running = false;
};
