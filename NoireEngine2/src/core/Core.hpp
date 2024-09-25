#pragma once

// useful macros
#define BIT(x) (1 << x)

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

#define NE_ENTITY_TYPE "NEntity"
#define NE_NULL_STR "<NONE>"
