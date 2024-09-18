#include "Component.hpp"
#include <iostream>

#include "Entity.hpp"

Transform* Component::GetTransform()
{
	if (!entity) {
		std::cerr << "Entity is null!";
		return nullptr;
	}

	return entity->transform();
}
