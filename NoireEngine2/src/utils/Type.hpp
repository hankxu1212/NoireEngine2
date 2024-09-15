#pragma once

#include <typeindex>
#include <typeinfo>
#include <unordered_map>

using TypeId = std::size_t;

template<typename T>
class Type {
public:
	Type() = delete;

	/**
	  * Obtain a unique type ID of K which is a base of T.
	  * @tparam K The type ID K.
	  * @return The unique type ID, which is basically a hash of class K
	*/
	template<typename K,
		typename = std::enable_if_t<std::is_convertible_v<K*, T*>>>
	static TypeId GetTypeId() noexcept 
	{
		std::type_index typeIndex = typeid(K);

		if (auto it = typeMap.find(typeIndex); it != typeMap.end())
			return it->second;

		const TypeId id = NextTypeId();
		typeMap[typeIndex] = id;
		return id;
	}

private:
	static TypeId NextTypeId() noexcept 
	{
		const TypeId id = nextTypeId;
		++nextTypeId;
		return id;
	}

	// Next type ID for T.
	static inline TypeId nextTypeId;
	static inline std::unordered_map<std::type_index, TypeId> typeMap;
};
