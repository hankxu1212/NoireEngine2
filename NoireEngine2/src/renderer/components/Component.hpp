#pragma once

#include "renderer/scene/Transform.hpp"

class Entity;
class Scene;

class Component
{
public:
	virtual void Update() {}
	virtual void Render(const glm::mat4& model) {}

	void SetEntity(Entity*);

	Transform* GetTransform();
	Scene* GetScene();

protected:
	Entity* entity;
};
