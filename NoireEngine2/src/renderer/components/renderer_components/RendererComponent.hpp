#pragma once

#include "renderer/components/Component.hpp"

#include "renderer/object/Mesh.hpp"

class RendererComponent : public Component
{
public:
	RendererComponent(Mesh* mesh_);

public:
	virtual void Update() override;
	virtual void Render(const glm::mat4& model) override;

	Mesh* mesh;
};
