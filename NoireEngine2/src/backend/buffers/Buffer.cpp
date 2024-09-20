#include <array>

#include "Buffer.hpp"
#include "backend/VulkanContext.hpp"

Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, MapFlag map)
{
	CreateBuffer(size, usage, properties, map);
}

void Buffer::Destroy()
{
	if (buffer == VK_NULL_HANDLE || bufferMemory == VK_NULL_HANDLE)
		return;

	if (mapped != nullptr) {
		UnmapMemory();
		mapped = nullptr;
	}

	VulkanContext::VK_CHECK(vkQueueWaitIdle(VulkanContext::Get()->getLogicalDevice()->getGraphicsQueue()), "[vulkan] wait idle fail on destroying buffer");
	vkDestroyBuffer(VulkanContext::GetDevice(), buffer, nullptr);
	vkFreeMemory(VulkanContext::GetDevice(), bufferMemory, nullptr);

	buffer = VK_NULL_HANDLE;
	bufferMemory = VK_NULL_HANDLE;
	m_Size = 0;
}

void Buffer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, MapFlag map)
{
	m_Size = size;
	auto& logicalDevice = *(VulkanContext::Get()->getLogicalDevice());

	std::array<uint32_t, 3> queueFamily = {
		logicalDevice.getGraphicsFamily(),
		logicalDevice.getPresentFamily(),
		logicalDevice.getComputeFamily()
	};

	// Create the buffer handle.
	VkBufferCreateInfo bufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = static_cast<uint32_t>(queueFamily.size()),
		.pQueueFamilyIndices = queueFamily.data()
	};
	VulkanContext::VK_CHECK(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &buffer),
		"[vulkan] Error: failed to create buffer");

	// Create the memory backing up the buffer handle.
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(logicalDevice, buffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = VulkanContext::FindMemoryType(memoryRequirements.memoryTypeBits, properties)
	};
	VulkanContext::VK_CHECK(vkAllocateMemory(logicalDevice, &memoryAllocateInfo, nullptr, &bufferMemory),
		"[vulkan] Error: cannot allocate buffer memory");

	if (map == Mapped)
		MapMemory(&mapped);

	// Attach the memory to the buffer object.
	VulkanContext::VK_CHECK(vkBindBufferMemory(logicalDevice, buffer, bufferMemory, 0), "[vulkan] Error: cannot bind buffer memory");
}

void Buffer::MapMemory(void **data) const {
	VulkanContext::VK_CHECK(vkMapMemory(VulkanContext::GetDevice(), bufferMemory, 0, m_Size, 0, data), "[vulkan] Error: failed to map buffer memory");
}

void Buffer::UnmapMemory() const {
	vkUnmapMemory(VulkanContext::GetDevice(), bufferMemory);
}

void Buffer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	CommandBuffer commandBuffer;
	CopyBuffer(commandBuffer, srcBuffer, dstBuffer, size);
	commandBuffer.SubmitIdle();
}

void Buffer::CopyBuffer(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(cmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

void Buffer::TransferToBuffer(void* data, size_t size, VkBuffer dstBuffer)
{
	Buffer transferSource = Buffer(
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		Mapped
	);

	std::memcpy(transferSource.mapped, data, size);

	CopyBuffer(transferSource.buffer, dstBuffer, size);

	transferSource.Destroy();
}

VkBufferMemoryBarrier Buffer::CreateMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, uint32_t offset)
{
	return {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.buffer = buffer,
		.offset = offset,
		.size = m_Size
	};
}

