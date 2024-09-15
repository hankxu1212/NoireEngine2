/*****************************************************************//**
 * \file   Node.hpp
 * \brief  Used to serialize resources. A node represent a handle to a serialized engine resource
 * 
 * \author Hank Xu
 * \date   September 2024
 *********************************************************************/

#pragma once

#include <ostream>
#include <vector>

enum class NodeType : uint8_t {
	Object, Array, String, Boolean, Integer, Decimal, Null, // Type of node value.
	Unknown, Token, EndOfFile, // Used in tokenizers.
};

class Node
{
public:
	using NodeValue = std::string;
	using NodeProperty = std::pair<std::string, Node>;
public:
	Node() {}
	Node(const Node &node) = default;
	Node(Node &&node) = default;

	void Clear();
	bool IsValid() const;

	template<typename T>
	T Get() const;

	template<typename T>
	bool Get(T& dest) const;
	
	template<typename T>
	bool Get(T&& dest) const;
	
	template<typename T>
	void Set(const T& value);
	
	template<typename T>
	void Set(T&& value);

	Node& AddProperty(const Node& node);
	Node& AddProperty(Node&& node = {});
	Node& AddProperty(const std::string& name, const Node& node);
	Node& AddProperty(const std::string& name, Node&& node = {});
	Node& AddProperty(uint32_t index, const Node& node);
	Node& AddProperty(uint32_t index, Node&& node = {});
	Node RemoveProperty(const std::string& name);
	Node RemoveProperty(const Node& node);
	
	Node& operator=(const Node& rhs) = default;
	Node& operator=(Node&& rhs) noexcept = default;
	
	template<typename T>
	Node& operator=(const T& rhs);

	bool operator==(const Node& rhs) const;
	bool operator!=(const Node& rhs) const;
	bool operator<(const Node& rhs) const;

public:
	std::vector<NodeProperty> properties;
	NodeValue value;
	NodeType type = NodeType::Object;
};

#include "Node.inl"
