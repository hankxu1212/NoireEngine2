#include "Node.hpp"

#include "core/Core.hpp"

static const Node::NodeProperty NullNode = Node::NodeProperty("", Node() = nullptr);

void Node::Clear() 
{
	properties.clear();
}

bool Node::IsValid() const 
{
	switch (type) {
	case NodeType::Token:
	case NodeType::Unknown:
		return false;
	case NodeType::Object:
	case NodeType::Array:
		return !properties.empty();
	case NodeType::Null:
		return true;
	default:
		return !value.empty();
	}
}

template<typename T>
T Node::Get() const {
	T value;
	*this >> value;
	return value;
}

template<typename T>
bool Node::Get(T& dest) const {
	if (!IsValid())
		return false;

	*this >> dest;
	return true;
}

template<typename T>
bool Node::Get(T&& dest) const {
	if (!IsValid())
		return false;

	*this >> dest;
	return true;
}

template<typename T>
void Node::Set(const T& value) {
	*this << value;
}

template<typename T>
void Node::Set(T&& value) {
	*this << value;
}

bool Node::HasProperty(const std::string& name) const {
	for (const auto& [propertyName, property] : properties) {
		if (propertyName == name)
			return true;
	}

	return false;
}

bool Node::HasProperty(uint32_t index) const {
	return index < properties.size();
}

// TODO: Duplicate
Node::View Node::GetProperty(const std::string& name) {
	for (auto& [propertyName, property] : properties) {
		if (propertyName == name)
			return { this, name, &property };
	}
	return { this, name, nullptr };
}

// TODO: Duplicate
Node::View Node::GetProperty(uint32_t index) {
	if (index < properties.size())
		return { this, index, &properties[index].second };

	return { this, index, nullptr };
}

Node& Node::AddProperty(const Node& node) {
	type = NodeType::Array;
	return properties.emplace_back(NodeProperty("", node)).second;
}

Node& Node::AddProperty(Node&& node) {
	type = NodeType::Array;
	return properties.emplace_back(NodeProperty("", std::move(node))).second;
}

Node& Node::AddProperty(const std::string& name, const Node& node) {
	return properties.emplace_back(NodeProperty(name, node)).second;
}

Node& Node::AddProperty(const std::string& name, Node&& node) {
	return properties.emplace_back(NodeProperty(name, std::move(node))).second;
}

Node& Node::AddProperty(uint32_t index, const Node& node) {
	type = NodeType::Array;
	properties.resize(max(properties.size(), static_cast<std::size_t>(index + 1)), NullNode);
	return properties[index].second = node;
}

Node& Node::AddProperty(uint32_t index, Node&& node) {
	type = NodeType::Array;
	properties.resize(max(properties.size(), static_cast<std::size_t>(index + 1)), NullNode);
	return properties[index].second = std::move(node);
}

Node Node::RemoveProperty(const std::string& name) {
	for (auto it = properties.begin(); it != properties.end(); ) {
		if (it->first == name) {
			auto result = std::move(it->second);
			properties.erase(it);
			return result;
		}
		++it;
	}
	return {};
}

Node Node::RemoveProperty(const Node& node) {
	for (auto it = properties.begin(); it != properties.end(); ) {
		if (it->second == node) {
			auto result = std::move(it->second);
			it = properties.erase(it);
			return result;
		}
		++it;
	}
	return {};
}

Node::View Node::operator[](const std::string& name)
{
	return GetProperty(name);
}

Node::View Node::operator[](uint32_t index)
{
	return GetProperty(index);
}

bool Node::operator==(const Node& rhs) const {
	return value == rhs.value && properties.size() == rhs.properties.size() &&
		std::equal(properties.begin(), properties.end(), rhs.properties.begin(), [](const auto& left, const auto& right) {
		return left == right;
			});
}

bool Node::operator!=(const Node& rhs) const {
	return !operator==(rhs);
}

bool Node::operator<(const Node& rhs) const {
	if (value < rhs.value) return true;
	if (rhs.value < value) return false;

	if (properties < rhs.properties) return true;
	if (rhs.properties < properties) return false;

	return false;
}