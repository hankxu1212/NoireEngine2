#pragma once

// useful macros
#define BIT(x) (1 << x)

#define TIME_NOW std::chrono::high_resolution_clock::now()

#define NE_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

#include <thread>
using TID = std::thread::id;

template<class T>
const T& min(const T& a, const T& b)
{
	return (b < a) ? b : a;
}

template<class T>
const T& max(const T& a, const T& b)
{
	return (b > a) ? b : a;
}

#define PTR_ADD(ptr, offset) static_cast<void*>(static_cast<std::byte*>(ptr) + offset)

#define NE_ENTITY_TYPE "NEntity"
#define NE_NULL_STR "none"

// clang-format off
#ifdef __cplusplus // Descriptor binding helper for C++ and GLSL
#define START_BINDING(a) enum a {
#define END_BINDING() }
#else
#define START_BINDING(a)  const uint
#define END_BINDING() 
#endif