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

	Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, MapFlag map=Unmapped);

	void CreateBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, MapFlag map = Unmapped, void* memoryAllocationInfoPNext=nullptr);

	void MapMemory(void **data) const;

	void UnmapMemory();
	
	// executes a copy action command.
	static void CopyBuffer(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	// executes a memcpy and a copy buffer command
	static void CopyFromHost(VkCommandBuffer cmdBuffer, const Buffer& hostBuffer, Buffer& deviceBuffer, VkDeviceSize size, const void* data);
	
	// transfer to a device local buffer using memcpy. Allocates a new command buffer and idle submits.
	// This is quite slow cuz it waits idle on the graphics queue. Should NOT be called in a loop
	static void TransferToBufferIdle(void* data, size_t size, VkBuffer dstBuffer);

	VkDeviceAddress GetBufferDeviceAddress() const;

	VkDescriptorBufferInfo GetDescriptorInfo();

	void InsertBufferMemoryBarrier(const CommandBuffer& commandBuffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);

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

struct AddressedBuffer
{
	Buffer buffer;
	size_t deviceAddress = 0;

	void Destroy() {
		buffer.Destroy();
	}
};
