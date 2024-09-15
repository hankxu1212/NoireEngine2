#pragma once

#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <optional>
#include <utility>

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


template<typename T>
struct is_optional : std::false_type {
};

template<typename T>
struct is_optional<std::optional<T>> : std::true_type {
};

template<typename T>
inline constexpr bool is_optional_v = is_optional<T>::value;

template<typename T>
struct is_pair : std::false_type {
};

template<typename T, typename U>
struct is_pair<std::pair<T, U>> : std::true_type {
};

template<typename T>
inline constexpr bool is_pair_v = is_pair<T>::value;

template<typename T>
struct is_vector : std::false_type {
};

template<typename T, typename A>
struct is_vector<std::vector<T, A>> : std::true_type {
};

template<typename T>
inline constexpr bool is_vector_v = is_vector<T>::value;

template<typename T, typename U = void>
struct is_map : std::false_type {
};

template<typename T>
struct is_map<T, std::void_t<typename T::key_type, typename T::mapped_type, decltype(std::declval<T&>()[std::declval<const typename T::key_type&>()])>> : std::true_type {
};

template<typename T>
inline constexpr bool is_map_v = is_map<T>::value;

template<typename T>
struct is_unique_ptr : std::false_type {
};

template<typename T, typename D>
struct is_unique_ptr<std::unique_ptr<T, D>> : std::true_type {
};

template<typename T>
inline constexpr bool is_unique_ptr_v = is_unique_ptr<T>::value;

template<typename T>
struct is_shared_ptr : std::false_type {
};

template<typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {
};

template<typename T>
inline constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

template<typename T>
struct is_weak_ptr : std::false_type {
};

template<typename T>
struct is_weak_ptr<std::weak_ptr<T>> : std::true_type {
};

template<typename T>
inline constexpr bool is_weak_ptr_v = is_weak_ptr<T>::value;

template<typename T>
inline constexpr bool is_ptr_access_v = std::is_pointer_v<T> || is_unique_ptr_v<T> || is_shared_ptr_v<T> || is_weak_ptr_v<T>;