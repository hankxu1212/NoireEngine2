#pragma once
#pragma warning (disable: 4324)

#include "core/Core.hpp"
#include "math/color/Color.hpp"
#include "glm/gtx/string_cast.hpp"
#include "renderer/Camera.hpp"
#include "renderer/components/Component.hpp"

#include <variant>

//struct DirectionalLight {
//	alignas(16) glm::vec3 color;
//	alignas(16) glm::vec3 direction;
//	alignas(16) glm::vec4 intensities;
//	alignas(16) glm::vec4 shadowParams;
//	alignas(16) glm::vec4 softShadows;
//};
////static_assert(sizeof(DirectionalLight) == 16 * 5);
//
//struct PointLight {
//	alignas(16) glm::vec3 color;
//	alignas(16) glm::vec3 position;
//	alignas(16) glm::vec3 attenuation;
//	alignas(16) glm::vec4 intensities;
//	alignas(16) glm::vec4 shadowParams;
//	alignas(16) glm::vec4 softShadows;
//};
////static_assert(sizeof(PointLight) == 16 * 6);

struct LightUniform {
	alignas(16) uint32_t type;
	alignas(16) glm::vec3 color;
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 direction;
	alignas(16) glm::vec2 cutoffs;
	alignas(16) glm::vec3 attenuation;
	alignas(16) glm::vec4 intensities;
	alignas(16) glm::vec4 shadowParams;
	alignas(16) glm::vec4 softShadows;
};
static_assert(sizeof(LightUniform) == 16 * 9);

class Light : public Component
{
public:
	enum Type { Directional, Point, Spot, };

public:
	Type type = Directional;

	void Update() override;

	_NODISCARD LightUniform& GetLightUniform() { return m_LightUniform; }

private:
	LightUniform m_LightUniform;
};