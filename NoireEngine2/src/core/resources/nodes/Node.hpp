#pragma once

#include <ostream>

#include "NodeView.hpp"

/**
 * @brief Class that is used to represent a tree of UFT-8 values, used in serialization.
 */
class Node final {
public:
	Node() {} // = default;
	Node(const Node &node) = default;
	Node(Node &&node) noexcept = default;

	template<typename T>
	T Get() const;
	template<typename T>
	T GetWithFallback(const T &fallback) const;
	template<typename T>
	bool Get(T &dest) const;
	template<typename T, typename K>
	bool GetWithFallback(T &dest, const K &fallback) const;
	template<typename T>
	bool Get(T &&dest) const;
	template<typename T, typename K>
	bool GetWithFallback(T &&dest, const K &fallback) const;
	template<typename T>
	void Set(const T &value);
	template<typename T>
	void Set(T &&value);
	
	/**
	 * Clears all properties from this node.
	 */
	void Clear();

	/**
	 * Gets if the node has a value, or has properties that have values.
	 * @return If the node is internally valid.
	 */
	bool IsValid() const;

	template<typename T>
	Node &Append(const T &value);
	template<typename ...Args>
	Node &Append(const Args &...args);
	
	//Node &Merge(Node &&node);

	bool HasProperty(const std::string &name) const;
	bool HasProperty(uint32_t index) const;
	NodeConstView GetProperty(const std::string &name) const;
	NodeConstView GetProperty(uint32_t index) const;
	NodeView GetProperty(const std::string &name);
	NodeView GetProperty(uint32_t index);
	Node &AddProperty(const Node &node);
	Node &AddProperty(Node &&node = {});
	Node &AddProperty(const std::string &name, const Node &node);
	Node &AddProperty(const std::string &name, Node &&node = {});
	Node &AddProperty(uint32_t index, const Node &node);
	Node &AddProperty(uint32_t index, Node &&node = {});
	Node RemoveProperty(const std::string &name);
	Node RemoveProperty(const Node &node);

	NodeConstView GetPropertyWithBackup(const std::string &name, const std::string &backupName) const;
	NodeConstView GetPropertyWithValue(const std::string &name, const NodeValue &propertyValue) const;
	NodeView GetPropertyWithBackup(const std::string &name, const std::string &backupName);
	NodeView GetPropertyWithValue(const std::string &name, const NodeValue &propertyValue);

	NodeConstView operator[](const std::string &name) const;
	NodeConstView operator[](uint32_t index) const;
	NodeView operator[](const std::string &name);
	NodeView operator[](uint32_t index);

	Node &operator=(const Node &rhs) = default;
	Node &operator=(Node &&rhs) noexcept = default;
	Node &operator=(const NodeConstView &rhs);
	Node &operator=(NodeConstView &&rhs);
	Node &operator=(NodeView &rhs);
	Node &operator=(NodeView &&rhs);
	template<typename T>
	Node &operator=(const T &rhs);

	bool operator==(const Node &rhs) const;
	bool operator!=(const Node &rhs) const;
	bool operator<(const Node &rhs) const;

	const NodeProperties &GetProperties() const { return properties; }
	NodeProperties &GetProperties() { return properties; }

	const NodeValue &GetValue() const { return value; }
	void SetValue(NodeValue v) { this->value = std::move(v); }

	const NodeType &GetType() const { return type; }
	void SetType(NodeType t) { this->type = t; }

protected:
	NodeProperties properties;
	NodeValue value;
	NodeType type = NodeType::Object;
};

#include "Node.inl"
#include "NodeConstView.inl"
#include "NodeView.inl"
