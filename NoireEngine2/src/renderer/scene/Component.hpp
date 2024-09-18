#pragma once

#include "Transform.hpp"

class Entity;

class Component
{
public:
	virtual void Update() {}

	Transform* GetTransform();

private:
	Entity* entity;
};

