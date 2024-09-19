#include "Component.hpp"
#include <iostream>

#include "renderer/scene/Entity.hpp"

Transform* Component::GetTransform()
{
	if (!entity) {
		std::cerr << "Entity is null!";
		return nullptr;
	}

	return entity->transform();
}
