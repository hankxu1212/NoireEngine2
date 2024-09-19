#include "Component.hpp"
#include <iostream>

#include "renderer/scene/Entity.hpp"

void Component::SetEntity(Entity* thisEntity)
{
	entity = thisEntity;
}

Transform* Component::GetTransform()
{
	if (!entity) {
		std::cerr << "Entity is null!";
		return nullptr;
	}

	return entity->transform();
}
