#include <array>

#include "Buffer.hpp"
#include "backend/VulkanContext.hpp"

Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, MapFlag map) :
	m_Size(size)
{
	if (usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
	{
		VkMemoryAllocateFlagsInfoKHR flags_info{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR };
		flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

		CreateBuffer(usage, properties, map, &flags_info);
	}
	else
		CreateBuffer(usage, properties, map);
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

void Buffer::CreateBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, MapFlag map, void* memoryAllocationInfoPNext)
{
	// Create the buffer handle.
	VkBufferCreateInfo bufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = m_Size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	VulkanContext::VK(vkCreateBuffer(VulkanContext::GetDevice(), &bufferCreateInfo, nullptr, &buffer));

	// Create the memory backing up the buffer handle.
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(VulkanContext::GetDevice(), buffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = memoryAllocationInfoPNext,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = VulkanContext::FindMemoryType(memoryRequirements.memoryTypeBits, properties),
	};
	VulkanContext::VK(vkAllocateMemory(VulkanContext::GetDevice(), &memoryAllocateInfo, nullptr, &bufferMemory));

	if (map == Mapped)
		MapMemory(&mapped);

	// Attach the memory to the buffer object.
	VulkanContext::VK(vkBindBufferMemory(VulkanContext::GetDevice(), buffer, bufferMemory, 0));
}

void Buffer::MapMemory(void **data) const 
{
	VulkanContext::VK(vkMapMemory(VulkanContext::GetDevice(), bufferMemory, 0, m_Size, 0, data));
}

void Buffer::UnmapMemory() 
{
	vkUnmapMemory(VulkanContext::GetDevice(), bufferMemory);
	mapped = nullptr;
}

void Buffer::CopyBuffer(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	if (size == 0)
		return;

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(cmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

void Buffer::CopyFromHost(VkCommandBuffer cmdBuffer, const Buffer& hostBuffer, Buffer& deviceBuffer, VkDeviceSize size, const void* data)
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

VkDeviceAddress Buffer::GetBufferDeviceAddress() const
{
	VkBufferDeviceAddressInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, buffer };
	return vkGetBufferDeviceAddress(VulkanContext::GetDevice(), &bufferInfo);
}

VkDescriptorBufferInfo Buffer::GetDescriptorInfo()
{
	return VkDescriptorBufferInfo {
		.buffer = buffer,
		.offset = 0,
		.range = m_Size
	};
}

void Buffer::InsertBufferMemoryBarrier(const CommandBuffer& commandBuffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage)
{
	VkBufferMemoryBarrier memoryBarrier = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.srcAccessMask = srcAccessMask,
		.dstAccessMask = dstAccessMask,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.buffer = buffer,
		.offset = 0,
		.size = m_Size
	};

	vkCmdPipelineBarrier(
		commandBuffer,
		srcStage,
		dstStage,
		0,
		0, nullptr,
		1, &memoryBarrier,
		0, nullptr
	);
}

