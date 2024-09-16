#include <array>

#include "Buffer.hpp"
#include "backend/VulkanContext.hpp"

Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const void* data) :
	size(size)
{
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

	VkMemoryAllocateInfo memoryAllocateInfo {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = VulkanContext::FindMemoryType(memoryRequirements.memoryTypeBits, properties)
	};
	VulkanContext::VK_CHECK(vkAllocateMemory(logicalDevice, &memoryAllocateInfo, nullptr, &bufferMemory),
		"[vulkan] Error: cannot allocate buffer memory");

	// If a pointer to the buffer data has been passed, map the buffer and copy over the data.
	if (data) {
		void *mapped;
		MapMemory(&mapped);
		std::memcpy(mapped, data, size);

		// If host coherency hasn't been requested, do a manual flush to make writes visible.
		if ((properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
			VkMappedMemoryRange mappedMemoryRange = {
				.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
				.memory = bufferMemory,
				.offset = 0,
				.size = size
			};
			vkFlushMappedMemoryRanges(logicalDevice, 1, &mappedMemoryRange);
		}

		UnmapMemory();
	}

	// Attach the memory to the buffer object.
	VulkanContext::VK_CHECK(vkBindBufferMemory(logicalDevice, buffer, bufferMemory, 0), "[vulkan] Error: cannot bind buffer memory");
}

Buffer::~Buffer() 
{
	VulkanContext::VK_CHECK(vkQueueWaitIdle(VulkanContext::Get()->getLogicalDevice()->getGraphicsQueue()), "[vulkan] wait idle fail on destroying buffer");
	vkDestroyBuffer(VulkanContext::GetDevice(), buffer, nullptr);
	vkFreeMemory(VulkanContext::GetDevice(), bufferMemory, nullptr);
}

void Buffer::MapMemory(void **data) const {
	VulkanContext::VK_CHECK(vkMapMemory(VulkanContext::GetDevice(), bufferMemory, 0, size, 0, data), "[vulkan] Error: failed to map buffer memory");
}

void Buffer::UnmapMemory() const {
	vkUnmapMemory(VulkanContext::GetDevice(), bufferMemory);
}

void Buffer::InsertBufferMemoryBarrier(const CommandBuffer &commandBuffer, const VkBuffer &buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
	VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDeviceSize offset, VkDeviceSize size) {
	VkBufferMemoryBarrier bufferMemoryBarrier {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = srcAccessMask,
		.dstAccessMask = dstAccessMask,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.buffer = buffer,
		.offset = offset,
		.size = size
	};
	vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr);
}

void Buffer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	CommandBuffer commandBuffer;

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	commandBuffer.SubmitIdle();
}
