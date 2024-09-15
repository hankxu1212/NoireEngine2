#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <optional>

struct OffsetSize 
{
	OffsetSize(uint32_t _offset, uint32_t _size) : offset(_offset), size(_size) {}

	bool operator==(const OffsetSize& rhs) const {
		return offset == rhs.offset && size == rhs.size;
	}

	bool operator!=(const OffsetSize& rhs) const {
		return !operator==(rhs);
	}

	uint32_t offset;
	uint32_t size;
};

class WriteDescriptorSet 
{
public:
	WriteDescriptorSet(const VkWriteDescriptorSet& descriptorSet, const VkDescriptorImageInfo& imageInfo) :
		m_WriteDescriptorSet(descriptorSet),
		s_ImageInfo(std::make_unique<VkDescriptorImageInfo>(imageInfo))
	{
		this->m_WriteDescriptorSet.pImageInfo = this->s_ImageInfo.get();
	}

	WriteDescriptorSet(const VkWriteDescriptorSet& descriptorSet, const VkDescriptorBufferInfo& bufferInfo) :
		m_WriteDescriptorSet(descriptorSet),
		s_BufferInfo(std::make_unique<VkDescriptorBufferInfo>(bufferInfo))
	{
		this->m_WriteDescriptorSet.pBufferInfo = this->s_BufferInfo.get();
	}

	const VkWriteDescriptorSet& getWriteDescriptorSet() const { return m_WriteDescriptorSet; }

private:
	VkWriteDescriptorSet						m_WriteDescriptorSet;
	std::unique_ptr<VkDescriptorImageInfo>		s_ImageInfo;
	std::unique_ptr<VkDescriptorBufferInfo>		s_BufferInfo;
};

class Descriptor {
public:
	Descriptor() = default;
	virtual ~Descriptor() = default;

	virtual WriteDescriptorSet getWriteDescriptor(uint32_t binding, VkDescriptorType descriptorType, const std::optional<OffsetSize>& offsetSize) const = 0;
};
