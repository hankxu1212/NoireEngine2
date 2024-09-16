#pragma once

#include <vulkan/vulkan.h>

class CommandBuffer;

class Buffer {
public:
	enum class Status {
		Reset, Changed, Normal
	};

	/**
	  * Creates a new buffer with optional data.
	  * @param size Size of the buffer in bytes.
	  * @param usage Usage flag bitmask for the buffer (i.e. index, vertex, uniform buffer).
	  * @param properties Memory properties for this buffer (i.e. device local, host visible, coherent).
	  * @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over).
	*/
	Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const void *data = nullptr);
	virtual ~Buffer();

	void MapMemory(void **data) const;
	void UnmapMemory() const;

	VkDeviceSize				getSize() const { return size; }
	const VkBuffer&				getBuffer() const { return buffer; }
	const VkDeviceMemory&		getBufferMemory() const { return bufferMemory; }

	static void InsertBufferMemoryBarrier(const CommandBuffer &commandBuffer, const VkBuffer &buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
		VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
		
	static void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

protected:
	VkBuffer buffer;
	VkDeviceSize size;
	VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
};
