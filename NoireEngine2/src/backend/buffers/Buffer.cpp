#include <array>

#include "Buffer.hpp"
#include "backend/VulkanContext.hpp"

Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, MapFlag map, void* memoryAllocationInfoPNext)
{
	CreateBuffer(size, usage, properties, map, memoryAllocationInfoPNext);
}

void Buffer::Destroy()
{
	if (buffer == VK_NULL_HANDLE || bufferMemory == VK_NULL_HANDLE)
		return;

	if (mapped != nullptr)
		UnmapMemory();

	vkDestroyBuffer(VulkanContext::GetDevice(), buffer, nullptr);
	vkFreeMemory(VulkanContext::GetDevice(), bufferMemory, nullptr);

	buffer = VK_NULL_HANDLE;
	bufferMemory = VK_NULL_HANDLE;
	m_Size = 0;
}

void Buffer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, MapFlag map, void* memoryAllocationInfoPNext)
{
	m_Size = size;
	auto& logicalDevice = *(VulkanContext::Get()->getLogicalDevice());

	// Create the buffer handle.
	VkBufferCreateInfo bufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	VulkanContext::VK(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &buffer),
		"[vulkan] Error: failed to create buffer");

	// Create the memory backing up the buffer handle.
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(logicalDevice, buffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = memoryAllocationInfoPNext,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = VulkanContext::FindMemoryType(memoryRequirements.memoryTypeBits, properties),
	};
	VulkanContext::VK(vkAllocateMemory(logicalDevice, &memoryAllocateInfo, nullptr, &bufferMemory),
		"[vulkan] Error: cannot allocate buffer memory");

	if (map == Mapped)
		MapMemory(&mapped);

	// Attach the memory to the buffer object.
	VulkanContext::VK(vkBindBufferMemory(logicalDevice, buffer, bufferMemory, 0), "[vulkan] Error: cannot bind buffer memory");
}

void Buffer::MapMemory(void **data) const {
	VulkanContext::VK(vkMapMemory(VulkanContext::GetDevice(), bufferMemory, 0, m_Size, 0, data), "[vulkan] Error: failed to map buffer memory");
}

void Buffer::UnmapMemory() {
	vkUnmapMemory(VulkanContext::GetDevice(), bufferMemory);
	mapped = nullptr;
}

void Buffer::CopyBuffer(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(cmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

void Buffer::CopyFromCPU(VkCommandBuffer cmdBuffer, const Buffer& hostBuffer, Buffer& deviceBuffer, VkDeviceSize size, const void* data)
{
	memcpy(hostBuffer.data(), data, size);
	CopyBuffer(cmdBuffer, hostBuffer.getBuffer(), deviceBuffer.getBuffer(), size);
}

void Buffer::TransferToBufferIdle(void* data, size_t size, VkBuffer dstBuffer)
{
	Buffer transferSource = Buffer(
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		Mapped
	);

	std::memcpy(transferSource.mapped, data, size);

	CommandBuffer commandBuffer(true, VK_QUEUE_TRANSFER_BIT);

	CopyBuffer(commandBuffer, transferSource.getBuffer(), dstBuffer, size);

	commandBuffer.SubmitIdle();
	transferSource.Destroy();
}

VkDescriptorBufferInfo Buffer::GetDescriptorInfo()
{
	return VkDescriptorBufferInfo {
		.buffer = buffer,
		.offset = 0,
		.range = m_Size
	};
}

VkBufferMemoryBarrier Buffer::CreateBufferMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, uint32_t offset)
{
	return {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.srcAccessMask = srcAccessMask,
		.dstAccessMask = dstAccessMask,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.buffer = buffer,
		.offset = offset,
		.size = m_Size
	};
}

