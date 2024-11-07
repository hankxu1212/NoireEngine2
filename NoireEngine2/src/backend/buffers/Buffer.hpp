#pragma once

#include <vulkan/vulkan.h>

class CommandBuffer;

// Buffer with NO automatic memory management
// NOTE: You HAVE TO deallocate manually
class Buffer {
public:
	enum MapFlag {
		Unmapped = 0,
		Mapped = 1,
	};
	
	Buffer() = default;

	// destroys the buffer, and waits on a queue before doing so
	void Destroy();

	Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, MapFlag map=Unmapped, void* memoryAllocationInfoPNext = nullptr);

	/**
	  * Creates a new buffer with optional data.
	  * @param size Size of the buffer in bytes.
	  * @param usage Usage flag bitmask for the buffer (i.e. index, vertex, uniform buffer).
	  * @param properties Memory properties for this buffer (i.e. device local, host visible, coherent).
	  * @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over).
	*/
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, MapFlag map = Unmapped, void* memoryAllocationInfoPNext=nullptr);

	void MapMemory(void **data) const;

	void UnmapMemory();
	
	// executes a copy action command.
	static void CopyBuffer(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	// executes a memcpy and a copy buffer command
	static void CopyFromCPU(VkCommandBuffer cmdBuffer, const Buffer& hostBuffer, Buffer& deviceBuffer, VkDeviceSize size, const void* data);
	
	// transfer to a device local buffer using memcpy. Allocates a new command buffer and idle submits.
	// This is quite slow cuz it waits idle on the graphics queue. Should NOT be called in a loop
	static void TransferToBufferIdle(void* data, size_t size, VkBuffer dstBuffer);

	VkDescriptorBufferInfo GetDescriptorInfo();

	VkBufferMemoryBarrier CreateBufferMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, uint32_t offset=0);

public:
	VkDeviceSize				getSize() const { return m_Size; }
	VkBuffer					getBuffer() const { return buffer; }
	const VkDeviceMemory&		getBufferMemory() const { return bufferMemory; }
	void*						data() const { return mapped; }

protected:
	VkBuffer				buffer = VK_NULL_HANDLE;
	VkDeviceSize			m_Size = 0;
	VkDeviceMemory			bufferMemory = VK_NULL_HANDLE;
	void*					mapped = nullptr;
};
