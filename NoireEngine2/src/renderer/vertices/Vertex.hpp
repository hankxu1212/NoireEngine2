#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <string>
#include <vector>
#include "core/resources/Resources.hpp"
#include "backend/commands/CommandBuffer.hpp"

struct VertexInput : Resource
{
	struct Attribute
	{
		std::string			src;
		uint32_t			offset = 0;
		uint32_t			stride = 0;
		std::string			format;

		Attribute() = default;

		Attribute(const std::string& src_, uint32_t offset_, uint32_t stride_, const std::string& format_)
		{
			src.assign(src_);
			offset = offset_;
			stride = stride_;
			format.assign(format_);
		}

		Attribute(const Attribute& other) 
		{
			src.assign(other.src);
			offset = other.offset;
			stride = other.stride;
			format.assign(other.format);
		}

		bool operator==(const Attribute& rhs) {
			return offset == rhs.offset && stride == rhs.stride && format.compare(rhs.format) == 0;
		}

		bool operator!=(const Attribute& rhs) {
			return !operator==(rhs);
		}
	};

	friend const Node& operator>>(const Node& node, Attribute& vertex);
	friend Node& operator<<(Node& node, const Attribute& vertex);

	friend const Node& operator>>(const Node& node, VertexInput& vertex);
	friend Node& operator<<(Node& node, const VertexInput& vertex);

	bool operator==(const VertexInput& rhs);
	bool operator!=(const VertexInput& rhs);

	virtual std::type_index getTypeIndex() const { return typeid(VertexInput); }

	void Initialize();

	void Bind(const CommandBuffer& commandBuffer);

	std::vector<Attribute>								m_NativeAttributes;
	std::vector<VkVertexInputBindingDescription2EXT>	m_Binding;
	std::vector<VkVertexInputAttributeDescription2EXT>  m_VulkanAttributes;
};

