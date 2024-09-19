#pragma once

#include "renderer/scene/Transform.hpp"

class Entity;

class Component
{
public:
	virtual void Update() {}

	void SetEntity(Entity*);
	Transform* GetTransform();

protected:
	Entity* entity;
};