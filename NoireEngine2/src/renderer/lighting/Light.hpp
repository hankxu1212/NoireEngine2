#pragma once
#pragma warning (disable: 4324)

#include "core/Core.hpp"
#include "math/color/Color.hpp"
#include "glm/gtx/string_cast.hpp"
#include "renderer/components/Component.hpp"
#include "renderer/gizmos/GizmosInstance.hpp"

#include <variant>

struct alignas(16) LightInfo 
{
	glm::vec4 color = { 1,1,1,0 };
	glm::vec4 position;
	glm::vec4 direction;
	float radius = 1;
	float limit = 10;
	float intensity = 1; /* or power */
	float fov = 0.349066f;
	float blend = 0.5f;
	uint32_t type = 0 /*Directional*/;
};
static_assert(sizeof(LightInfo) == 16 * 5);

struct alignas(16) DirectionalLightUniform
{
	glm::vec4 color = { 1,1,1,0 };
	glm::vec4 direction;
	float angle;
	float intensity = 1; /* or power */
};
static_assert(sizeof(DirectionalLightUniform) == 16 * 3);

struct alignas(16) PointLightUniform
{
	glm::vec4 color = { 1,1,1,0 };
	glm::vec4 position;
	float intensity = 1; /* or power */
	float radius = 1;
	float limit = 10;
};
static_assert(sizeof(PointLightUniform) == 16 * 3);

struct alignas(16) SpotLightUniform
{
	glm::vec4 color = { 1,1,1,0 };
	glm::vec4 position;
	glm::vec4 direction;
	float intensity = 1; /* or power */
	float radius = 1;
	float limit = 10;
	float fov = 0.349066f;
	float blend = 0.5f;
};
static_assert(sizeof(SpotLightUniform) == 16 * 5);


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

	_NODISCARD LightInfo& GetLightInfo() { return m_Info; }

	template<typename T>
	_NODISCARD T GetLightUniformAs() const;

	template<>
	_NODISCARD DirectionalLightUniform GetLightUniformAs() const
	{
		DirectionalLightUniform uniform;
		uniform.color = m_Info.color;
		uniform.direction = m_Info.direction;
		uniform.angle = 0.0f;
		uniform.intensity = m_Info.intensity;

		return uniform;
	}

	template<>
	_NODISCARD PointLightUniform GetLightUniformAs() const
	{
		PointLightUniform uniform;
		uniform.color = m_Info.color;
		uniform.position = m_Info.position;
		uniform.intensity = m_Info.intensity;
		uniform.radius = m_Info.radius;
		uniform.limit = m_Info.limit;

		return uniform;
	}

	template<>
	_NODISCARD SpotLightUniform GetLightUniformAs() const
	{
		SpotLightUniform uniform;
		uniform.color = m_Info.color;
		uniform.position = m_Info.position;
		uniform.direction = m_Info.direction;
		uniform.intensity = m_Info.intensity;
		uniform.radius = m_Info.radius;
		uniform.limit = m_Info.limit;
		uniform.fov = m_Info.fov;
		uniform.blend = m_Info.blend;

		return uniform;
	}

	const char* getName() override { return "Light"; }

private:
	LightInfo m_Info;
	GizmosInstance gizmos;
};