#pragma once
#pragma warning (disable: 4324)

#include "core/Core.hpp"
#include "math/color/Color.hpp"
#include "glm/gtx/string_cast.hpp"
#include "renderer/components/Component.hpp"
#include "renderer/gizmos/GizmosInstance.hpp"

#include <variant>

struct alignas(16) LightUniform 
{
	struct { float x, y, z, padding_; } color = { 1,1,1,0 };
	struct { float x, y, z, padding_; } position;
	struct { float x, y, z, padding_; } direction;
	float radius = 1;
	float limit = 10;
	float intensity = 1; /* or power */
	float fov = 0.349066f;
	float blend = 0.5f;
	uint32_t type = 0 /*Directional*/;
};
static_assert(sizeof(LightUniform) == 16 * 5);

class Light : public Component
{
public:
	enum class Type { Directional = 0, Point = 1, Spot = 2, };

	Light(Type type);
	Light(Type type, Color3 color, float intensity);
	Light(Type type, Color3 color, float intensity, float radius);
	Light(Type type, Color3 color, float intensity, float radius, float limit);
	Light(Type type, Color3 color, float intensity, float radius, float fov, float blend);
	Light(Type type, Color3 color, float intensity, float radius, float fov, float blend, float limit);

public:
	void Update() override;

	void Render(const glm::mat4& model) override;

	void Inspect() override;

	_NODISCARD LightUniform& GetLightUniform() { return m_LightUniform; }

	const char* getName() override { return "Light"; }

private:
	LightUniform m_LightUniform;
	GizmosInstance gizmos;
};