#pragma once
#pragma warning (disable: 4324)

#include "core/Core.hpp"
#include "math/color/Color.hpp"
#include "glm/gtx/string_cast.hpp"
#include "renderer/components/Component.hpp"
#include "renderer/gizmos/GizmosInstance.hpp"

#include <variant>

struct LightInfo 
{
	glm::vec4 color = { 1,1,1,0 };
	glm::vec4 position;
	glm::vec4 direction;
	float radius = 1;
	float limit = 100;
	float intensity = 1; /* or power */
	float fov = 0.349066f;
	float blend = 0.5f;
	uint32_t type = 0 /*Directional*/;

	// shadow params
	bool useShadows = true;
	float zNear = 1.0f;
	float zFar = 96.0f;
	glm::mat4 lightspace; /*depthMVP*/
	float oneMinusShadowStrength = 0;
};

struct alignas(16) DirectionalLightUniform
{
	glm::mat4 lightspace; /*depthMVP*/
	glm::vec4 color;
	glm::vec4 direction;
	float angle;
	float intensity;
	uint32_t shadowOffset;
	float shadowStrength;
};
static_assert(sizeof(DirectionalLightUniform) == 64 + 16 * 3);

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

	_NODISCARD inline LightInfo& GetLightInfo() { return m_Info; }

	template<typename T>
	_NODISCARD T GetLightUniformAs() const;

	const char* getName() override { return "Light"; }

private:
	LightInfo m_Info;
	GizmosInstance gizmos;
};