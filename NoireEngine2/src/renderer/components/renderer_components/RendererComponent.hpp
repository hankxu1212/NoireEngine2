#pragma once

#include "renderer/components/Component.hpp"

#include "renderer/object/Mesh.hpp"

class RendererComponent : public Component
{
	Mesh* mesh;
};
