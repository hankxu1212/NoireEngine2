#pragma once

#include <typeindex>

#include "utils/Singleton.hpp"

class Resource : Singleton 
{
public:
	Resource() = default;
	virtual ~Resource() = default;

	virtual std::type_index getTypeIndex() const = 0;

	inline uint32_t getID() const { return ID; }

private:
	friend class Resources;
	uint32_t ID;
};
