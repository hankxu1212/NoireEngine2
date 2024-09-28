#pragma once
#pragma warning (disable: 4324)

#include "core/Core.hpp"
#include "math/color/Color.hpp"
#include "glm/gtx/string_cast.hpp"
#include "renderer/Camera.hpp"
#include "renderer/components/Component.hpp"

#include <variant>

struct alignas(16) LightUniform 
{
	struct { float x, y, z, padding_; } color = { 1,1,1,0 };
	struct { float x, y, z, padding_; } position;
	struct { float x, y, z, padding_; } direction;
	struct { float x, y, z, padding_; } attenuation = { 1,1,1,0 };
	float innerCutoff = 20;
	float outerCutoff = 30;
	float intensity = 1;
	uint32_t type = 0 /*Directional*/;
};
static_assert(sizeof(LightUniform) == 16 * 5);

class Light : public Component
{
public:
	enum class Type { Directional = 0, Point = 1, Spot = 2, };

	Light(Type type);

	Light(Type type, Color3 color, float intensity);

public:
	Type type = Type::Directional;

	void Update() override;

	void Inspect() override;

	_NODISCARD LightUniform& GetLightUniform() { return m_LightUniform; }

	const char* getName() override { return "Light"; }

private:
	LightUniform m_LightUniform;
};