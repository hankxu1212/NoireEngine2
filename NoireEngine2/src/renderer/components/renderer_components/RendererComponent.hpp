#pragma once

#include "renderer/components/Component.hpp"

#include "renderer/object/Mesh.hpp"
#include "renderer/materials/Material.hpp"

class RendererComponent : public Component
{
public:
	RendererComponent(Mesh* mesh_);
	RendererComponent(Mesh* mesh_, Material* material_);

public:
	virtual void Update() override;
	virtual void Render(const glm::mat4& model) override;

	const char* getName() override { return "Mesh Renderer"; }

	Mesh* mesh = nullptr;
	Material* material = nullptr;
};
