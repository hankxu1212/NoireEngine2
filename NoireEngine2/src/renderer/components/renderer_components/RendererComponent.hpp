#pragma once

#include "renderer/components/Component.hpp"
#include "renderer/gizmos/GizmosInstance.hpp"

class Mesh;
class Material;

class RendererComponent : public Component
{
public:
	RendererComponent(Mesh* mesh_);
	RendererComponent(Mesh* mesh_, Material* material_);

public:
	virtual void Update() override;
	virtual void Render(const glm::mat4& model) override;

	const char* getName() override { return "Mesh Renderer"; }

	void Inspect() override;

	Mesh* mesh = nullptr;
	Material* material = nullptr;
	GizmosInstance gizmos;

private:
	friend class Entity;
	void PrepareAcceleration(const glm::mat4& model);
};
