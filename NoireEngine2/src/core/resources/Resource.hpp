#pragma once

#include <typeindex>

#include "utils/Singleton.hpp"

class Resource : Singleton {
public:
	Resource() = default;
	virtual ~Resource() = default;

	virtual std::type_index getTypeIndex() const = 0;
};
