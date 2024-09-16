/*****************************************************************//**
 * \file   Node.inl
 * \brief  This file contains different serialization methods for Nodes
 * 
 * \author Hank Xu
 * \date   September 2024
 *********************************************************************/

#pragma once

#include "Node.hpp"

#include <filesystem>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>

#include "core/resources/Resource.hpp"
#include "utils/Enumerate.hpp"
#include "utils/String.hpp"

template<typename T>
Node& Node::operator=(const T& rhs) {
	Set(rhs);
	return *this;
}

// Node objects
inline const Node& operator>>(const Node& node, Node& object) {
	object = node;
	return node;
}

inline Node& operator<<(Node& node, const Node& object) {
	node = object;
	return node;
}

// Null pointer objects
inline Node& operator<<(Node& node, const std::nullptr_t& object) {
	node.value = "";
	node.type = NodeType::Null;
	return node;
}

template<typename T, std::enable_if_t<std::is_pointer_v<T>, int> = 0>
Node& operator<<(Node& node, const T object) {
	if (!object)
		return node << nullptr;

	node << *object;
	return node;
}

// Unique pointer objects
template<typename T>
const Node& operator>>(const Node& node, std::unique_ptr<T>& object) {
	object = std::make_unique<T>();
	node >> *object;
	return node;
}

template<typename T>
Node& operator<<(Node& node, const std::unique_ptr<T>& object) {
	if (!object)
		return node << nullptr;

	node << *object;
	return node;
}

// Shared pointer objects
template<typename T>
const Node& operator>>(const Node& node, std::shared_ptr<T>& object) {
	// TODO: Abstract Resource streams out from shared_ptr.
	if constexpr (std::is_base_of_v<Resource, T>) {
		object = T::Create(node);
		return node;
	}
	else {
		object = std::make_shared<T>();
		node >> *object;
		return node;
	}
}

template<typename T>
Node& operator<<(Node& node, const std::shared_ptr<T>& object) {
	if (!object)
		return node << nullptr;

	node << *object;
	return node;
}

// Boolean objects
inline const Node& operator>>(const Node& node, bool& object) {
	object = String::From<bool>(node.value);
	return node;
}

inline Node& operator<<(Node& node, bool object) {
	node.value = String::To(object);
	node.type = NodeType::Boolean;
	return node;
}

// Enum objects
template<typename T, std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>, int> = 0>
const Node & operator>>(const Node & node, T & object) {
	object = String::From<T>(node.value);
	return node;
}

template<typename T, std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>, int> = 0>
Node & operator<<(Node & node, T object) {
	node.value = (String::To(object));
	node.type = (std::is_floating_point_v<T> ? NodeType::Decimal : NodeType::Integer);
	return node;
}

// String view objects
inline Node& operator<<(Node& node, std::string_view string) {
	node.value = (std::string(string));
	node.type = (NodeType::String);
	return node;
}

// String objects
inline const Node& operator>>(const Node& node, std::string& string) {
	string = node.value;
	return node;
}

inline Node& operator<<(Node& node, const std::string& string) {
	node.value = (string);
	node.type = (NodeType::String);
	return node;
}

// Wide string objects
inline const Node& operator>>(const Node& node, std::wstring& string) {
	string = String::ConvertUtf16(node.value);
	return node;
}

inline Node& operator<<(Node& node, const std::wstring& string) {
	node.value = (String::ConvertUtf8(string));
	node.type = (NodeType::String);
	return node;
}

// File system objects
inline const Node& operator>>(const Node& node, std::filesystem::path& object) {
	object = node.value;
	return node;
}

inline Node& operator<<(Node& node, const std::filesystem::path& object) {
	auto str = object.string();
	std::replace(str.begin(), str.end(), '\\', '/');
	node.value = (str);
	node.type = (NodeType::String);
	return node;
}

// Time objects
template<typename T, typename K>
const Node& operator>>(const Node& node, std::chrono::duration<T, K>& duration) {
	T x;
	node >> x;
	duration = std::chrono::duration<T, K>(x);
	return node;
}

template<typename T, typename K>
Node& operator<<(Node& node, const std::chrono::duration<T, K>& duration) {
	return node << duration.count();
}

// Time point objects
template<typename T>
const Node& operator>>(const Node& node, std::chrono::time_point<T>& timePoint) {
	typename std::chrono::time_point<T>::duration x;
	node >> x;
	timePoint = std::chrono::time_point<T>(x);
	return node;
}

template<typename T>
Node& operator<<(Node& node, const std::chrono::time_point<T>& timePoint) {
	return node << timePoint.time_since_epoch();
}

// Pair objects
template<typename T, typename K>
const Node& operator>>(const Node& node, std::pair<T, K>& pair) {
	pair.first = node["first"];
	pair.second = node["second"];
	return node;
}

template<typename T, typename K>
Node& operator<<(Node& node, const std::pair<T, K>& pair) {
	node["first"] = pair.first;
	node["second"] = pair.second;
	return node;
}

// Optional objects
template<typename T>
const Node& operator>>(const Node& node, std::optional<T>& optional) {
	if (node.type != NodeType::Null) {
		T x;
		node >> x;
		optional = std::move(x);
	}
	else {
		optional = std::nullopt;
	}

	return node;
}

template<typename T>
Node& operator<<(Node& node, const std::optional<T>& optional) {
	if (optional)
		return node << *optional;

	return node << nullptr;
}

// Vector objects
template<typename T>
const Node& operator>>(const Node& node, std::vector<T>& vector) {
	vector.clear();
	vector.reserve(node.properties.size());

	for (const auto& [propertyName, property] : node.properties)
		property >> vector.emplace_back();

	return node;
}

template<typename T>
Node& operator<<(Node& node, const std::vector<T>& vector) {
	for (const auto& x : vector)
		node.AddProperty() << x;

	node.type = (NodeType::Array);
	return node;
}

// Set objects
template<typename T>
const Node& operator>>(const Node& node, std::set<T>& set) {
	set.clear();
	auto where = set.end();

	for (const auto& [propertyName, property] : node.properties) {
		T x;
		property >> x;
		where = set.insert(where, std::move(x));
	}

	return node;
}

template<typename T>
Node& operator<<(Node& node, const std::set<T>& set) {
	for (const auto& x : set)
		node.AddProperty() << x;

	node.type = (NodeType::Array);
	return node;
}

// Unordered set objects
template<typename T>
const Node& operator>>(const Node& node, std::unordered_set<T>& set) {
	set.clear();
	auto where = set.end();

	for (const auto& [propertyName, property] : node.properties) {
		T x;
		property >> x;
		where = set.insert(where, std::move(x));
	}

	return node;
}

template<typename T>
Node& operator<<(Node& node, const std::unordered_set<T>& set) {
	for (const auto& x : set)
		node.AddProperty() << x;

	node.type = (NodeType::Array);
	return node;
}

// Multi-set objects
template<typename T>
const Node& operator>>(const Node& node, std::multiset<T>& set) {
	set.clear();
	auto where = set.end();

	for (const auto& [propertyName, property] : node.properties) {
		T x;
		property >> x;
		where = set.insert(where, std::move(x));
	}

	return node;
}

template<typename T>
Node& operator<<(Node& node, const std::multiset<T>& set) {
	for (const auto& x : set)
		node.AddProperty() << x;

	node.type = (NodeType::Array);
	return node;
}

// Unordered Multiset objects
template<typename T>
const Node& operator>>(const Node& node, std::unordered_multiset<T>& set) {
	set.clear();
	auto where = set.end();

	for (const auto& [propertyName, property] : node.properties) {
		T x;
		property >> x;
		where = set.insert(where, std::move(x));
	}

	return node;
}

template<typename T>
Node& operator<<(Node& node, const std::unordered_multiset<T>& set) {
	for (const auto& x : set)
		node.AddProperty() << x;

	node.type = (NodeType::Array);
	return node;
}

// Array objects
template<typename T, std::size_t Size>
const Node& operator>>(const Node& node, std::array<T, Size>& array) {
	array = {};

	for (auto [i, propertyPair] : Enumerate(node.properties))
		propertyPair.second >> array[i];

	return node;
}

template<typename T, std::size_t Size>
Node& operator<<(Node& node, const std::array<T, Size>& array) {
	for (const auto& x : array)
		node.AddProperty() << x;

	node.type = (NodeType::Array);
	return node;
}

// List objects
template<typename T>
const Node& operator>>(const Node& node, std::list<T>& list) {
	list.clear();

	for (const auto& [propertyName, property] : node.properties) {
		T x;
		property >> x;
		list.emplace_back(std::move(x));
	}

	return node;
}

template<typename T>
Node& operator<<(Node& node, const std::list<T>& list) {
	for (const auto& x : list)
		node.AddProperty() << x;

	node.type = (NodeType::Array);
	return node;
}

// Forward list objects
template<typename T>
const Node& operator>>(const Node& node, std::forward_list<T>& list) {
	list.clear();

	for (auto it = node.properties.rbegin(); it != node.properties.rend(); ++it) {
		T x;
		*it >> x;
		list.emplace_front(std::move(x));
	}

	return node;
}

template<typename T>
Node& operator<<(Node& node, const std::forward_list<T>& list) {
	for (const auto& x : list)
		node.AddProperty() << x;

	node.type = (NodeType::Array);
	return node;
}

// Map objects
template<typename T, typename K>
const Node& operator>>(const Node& node, std::map<T, K>& map) {
	map.clear();
	auto where = map.end();

	for (const auto& [propertyName, property] : node.properties) {
		std::pair<T, K> pair;
		pair.first = String::From<T>(propertyName);
		property >> pair.second;
		where = map.insert(where, std::move(pair));
	}

	return node;
}

template<typename T, typename K>
Node& operator<<(Node& node, const std::map<T, K>& map) {
	for (const auto& pair : map)
		node.AddProperty(String::To(pair.first)) << pair.second;

	node.type = (NodeType::Object);
	return node;
}

// Unordered map objects
template<typename T, typename K>
const Node& operator>>(const Node& node, std::unordered_map<T, K>& map) {
	map.clear();
	auto where = map.end();

	for (const auto& [propertyName, property] : node.properties) {
		std::pair<T, K> pair;
		pair.first = String::From<T>(propertyName);
		property >> pair.second;
		where = map.insert(where, std::move(pair));
	}

	return node;
}

template<typename T, typename K>
Node& operator<<(Node& node, const std::unordered_map<T, K>& map) {
	for (const auto& pair : map)
		node.AddProperty(String::To(pair.first)) << pair.second;

	node.type = (NodeType::Object);
	return node;
}

// Multi map objects
template<typename T, typename K>
const Node& operator>>(const Node& node, std::multimap<T, K>& map) {
	map.clear();
	auto where = map.end();

	for (const auto& [propertyName, property] : node.properties) {
		std::pair<T, K> pair;
		pair.first = String::From<T>(propertyName);
		property >> pair.second;
		where = map.insert(where, std::move(pair));
	}

	return node;
}

template<typename T, typename K>
Node& operator<<(Node& node, const std::multimap<T, K>& map) {
	for (const auto& pair : map)
		node.AddProperty(String::To(pair.first)) << pair.second;

	node.type = (NodeType::Object);
	return node;
}

// Unordered multi map objects
template<typename T, typename K>
const Node& operator>>(const Node& node, std::unordered_multimap<T, K>& map) {
	map.clear();
	auto where = map.end();

	for (const auto& [propertyName, property] : node.properties) {
		std::pair<T, K> pair;
		pair.first = String::From<T>(propertyName);
		property >> pair.second;
		where = map.insert(where, std::move(pair));
	}

	return node;
}

template<typename T, typename K>
Node& operator<<(Node& node, const std::unordered_multimap<T, K>& map) {
	for (const auto& pair : map)
		node.AddProperty(String::To(pair.first)) << pair.second;

	node.type = (NodeType::Object);
	return node;
}
