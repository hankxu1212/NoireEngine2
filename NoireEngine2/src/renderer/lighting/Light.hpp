#pragma once
#pragma warning (disable: 4324)

#include "core/Core.hpp"
#include "math/color/Color.hpp"
#include "glm/gtx/string_cast.hpp"
#include "renderer/components/Component.hpp"
#include "renderer/gizmos/GizmosInstance.hpp"

#include <variant>

#define SHADOW_MAP_CASCADE_COUNT 4

struct alignas(16) DirectionalLightUniform
{
	std::array<glm::mat4, SHADOW_MAP_CASCADE_COUNT> lightspaces; /*depthMVP*/
	glm::vec4 color;
	glm::vec4 direction;
	glm::vec4 splitDepths;
	float angle;
	float intensity;
	uint32_t shadowOffset;
	float shadowStrength;
};
static_assert(sizeof(DirectionalLightUniform) == 64 * 4 + 16 * 4);

struct alignas(16) PointLightUniform
{
	glm::mat4 lightspace; /*depthMVP*/
	glm::vec4 color;
	glm::vec4 position;
	float intensity;
	float radius;
	float limit;
	uint32_t shadowOffset;
	float shadowStrength;
};
static_assert(sizeof(PointLightUniform) == 64 + 16 * 2 + 16 * 2);

struct alignas(16) SpotLightUniform
{
	glm::mat4 lightspace; /*depthMVP*/
	glm::vec4 color;
	glm::vec4 position;
	glm::vec4 direction;
	float intensity;
	float radius;
	float limit;
	float fov;
	float blend;
	uint32_t shadowOffset;
	float shadowStrength;
	float nearClip;
};
static_assert(sizeof(SpotLightUniform) == 64 + 16 * 3 + 16 * 2);

class Light : public Component
{
public:
	using Info = std::variant<DirectionalLightUniform, PointLightUniform, SpotLightUniform>;

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

	template<typename T>
	_NODISCARD T GetLightUniformAs() const;

	const char* getName() override { return "Light"; }

	glm::vec4 m_Color = { 1,1,1,0 };
	glm::vec4 m_Position;
	glm::vec4 m_Direction;
	float m_Radius = 1;
	float m_Limit = 100;
	float m_Intensity = 1; /* or power */
	float m_Fov = 0.349066f;
	float m_Blend = 0.5f;
	uint32_t type = 0 /*Directional*/;

	// shadow params
	bool m_UseShadows = true;
	float m_NearClip = 1.0f;
	float m_FarClip = 96.0f;
	std::array<glm::mat4, SHADOW_MAP_CASCADE_COUNT> m_Lightspaces; /*depthMVP*/
	float m_ShadowAttenuation = 1;
	glm::vec4 m_CascadeSplitDepths;

private:
	void UpdateDirectionalLightCascades();
	
	GizmosInstance gizmos;
};