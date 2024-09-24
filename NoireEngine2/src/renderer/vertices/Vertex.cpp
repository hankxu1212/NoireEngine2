#include "Vertex.hpp"

#include "backend/VulkanContext.hpp"
#include <vulkan/vulkan_core.h>

#include <string>
#include <vulkan/utility/vk_format_utils.h>
#include <iostream>
#include <format>

static VkFormat vkStringToFormat(const std::string& formatStr)
{
	if (formatStr == "R32G32_SFLOAT")
		return VK_FORMAT_R32G32_SFLOAT;

	if (formatStr == "R32G32B32_SFLOAT")
		return VK_FORMAT_R32G32B32A32_SFLOAT;

	if (formatStr == "R32G32B32A32_SFLOAT")
		return VK_FORMAT_R32G32B32A32_SFLOAT;

	if (formatStr == "R8G8B8A8_UNORM")
		return VK_FORMAT_R8G8B8A8_UNORM;

	throw std::runtime_error(std::format("Could not decipher the format string: {}", formatStr));
}

const Node& operator>>(const Node& node, VertexInput::Attribute& vertex)
{
	node["offset"].Get(vertex.offset);
	node["stride"].Get(vertex.stride);
	node["format"].Get(vertex.format);
	return node;
}

Node& operator<<(Node& node, const VertexInput::Attribute& vertex)
{
	node["offset"].Set(vertex.offset);
	node["stride"].Set(vertex.stride);
	node["format"].Set(vertex.format);
	return node;
}

const Node& operator>>(const Node& node, VertexInput& vertex)
{
	node["attributes"].Get(vertex.m_NativeAttributes);
	return node;
}

Node& operator<<(Node& node, const VertexInput& vertex)
{
	node["attributes"].Set(vertex.m_NativeAttributes);
	return node;
}

bool VertexInput::operator==(const VertexInput& rhs)
{
	if (m_NativeAttributes.size() != rhs.m_NativeAttributes.size())
		return false;
	for (int i = 0; i < m_NativeAttributes.size(); ++i)
	{
		if (m_NativeAttributes[i] != rhs.m_NativeAttributes[i])
			return false;
	}
	return true;
}

bool VertexInput::operator!=(const VertexInput& rhs)
{
	return !operator==(rhs);
}

VertexInput::VertexInput(const std::vector<Attribute>& attributes)
{
	m_NativeAttributes.assign(attributes.begin(), attributes.end());
}

std::shared_ptr<VertexInput> VertexInput::Create(const std::vector<Attribute>& attributes)
{
	VertexInput temp(attributes);
	Node node;
	node << temp;
	return Create(node);
}

std::shared_ptr<VertexInput> VertexInput::Create(const Node& node)
{
	if (auto resource = Resources::Get()->Find<VertexInput>(node)) {
		return resource;
	}

	auto result = std::make_shared<VertexInput>();
	Resources::Get()->Add(node, std::dynamic_pointer_cast<Resource>(result));
	node >> *result;
	result->Load();
	return result;
}

void VertexInput::Load()
{
	uint32_t i = 0;

	for (const auto& nativeAttribute : m_NativeAttributes)
	{
		VkFormat format = vkStringToFormat(nativeAttribute.format);
		m_VulkanAttributes.emplace_back(VkVertexInputAttributeDescription2EXT{
			.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
			.location = i++,
			.binding = 0,
			.format = format,
			.offset = nativeAttribute.offset,
		});
	}

	m_Binding = {
		VkVertexInputBindingDescription2EXT
		{
			VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT,
			nullptr,
			0,
			m_NativeAttributes[0].stride,
			VK_VERTEX_INPUT_RATE_VERTEX,
			1
		}
	};
}

void VertexInput::Bind(const CommandBuffer& commandBuffer)
{
	static auto func = (PFN_vkCmdSetVertexInputEXT)vkGetInstanceProcAddr(*VulkanContext::Get()->getInstance(), "vkCmdSetVertexInputEXT");
	if (func == nullptr)
		std::runtime_error("Did not find vkCmdSetVertexInputEXT function pointer.\n");

	func(commandBuffer,
		static_cast<uint32_t>(m_Binding.size()),
		m_Binding.data(),
		static_cast<uint32_t>(m_VulkanAttributes.size()),
		m_VulkanAttributes.data());
}
