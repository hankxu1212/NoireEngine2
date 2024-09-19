#pragma once

#include "renderer/scene/Transform.hpp"

class Entity;

class Component
{
public:
	virtual void Update() {}

	Transform* GetTransform();

private:
	Entity* entity;
};