#include "Light.hpp"
#include "renderer/scene/Entity.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "backend/pipeline/ObjectPipeline.hpp"
#include "renderer/scene/SceneManager.hpp"
#include "imgui/imgui.h"

Light::Light(Type type)
{
	this->type = (uint32_t)type;
	useGizmos = true;
}

Light::Light(Type type, Color3 color, float intensity) :
	Light(type)
{
	memcpy(&this->m_Color, color.value_ptr(), sizeof(Color3));
	this->m_Intensity = intensity;
}

Light::Light(Type type, Color3 color, float intensity, float radius) :
	Light(type, color, intensity)
{
	this->m_Radius = radius;
}

Light::Light(Type type, Color3 color, float intensity, float radius, float limit) :
	Light(type, color, intensity, radius)
{
	this->m_Limit = limit;
}

Light::Light(Type type, Color3 color, float intensity, float radius, float fov, float blend) :
	Light(type, color, intensity, radius)
{
	this->m_Fov = fov;
	this->m_Blend = blend;
}

Light::Light(Type type, Color3 color, float intensity, float radius, float fov, float blend, float limit) :
	Light(type, color, intensity, radius, fov, blend)
{
	this->m_Limit = limit;
}


void Light::Update()
{
	Transform* transform = GetTransform();
	glm::vec3 dir = transform->Back(); // always point in -z m_Direction
	glm::vec3 pos = transform->WorldLocation();

	m_Direction = glm::vec4(dir, 0);
	m_Position = glm::vec4(pos, 0);

	if (m_UseShadows)
	{
		if (type == (uint32_t)Type::Directional)
		{
			UpdateDirectionalLightCascades();
		}
		else if (type == (uint32_t)Type::Point)
		{
			// TODO: add
		}
		else  // Type::Spot
		{
			glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(m_Fov), 1.0f, m_NearClip, m_FarClip);
			glm::mat4 depthViewMatrix = glm::lookAt(pos, pos + dir, transform->Up());
			m_Lightspaces[0] = depthProjectionMatrix * depthViewMatrix;
		}
	}
}

void Light::Render(const glm::mat4& model)
{
	if (!ObjectPipeline::UseGizmos || !useGizmos)
		return;

	Color4_4 c{
		static_cast<uint8_t>(m_Color.x * 255),
		static_cast<uint8_t>(m_Color.y * 255),
		static_cast<uint8_t>(m_Color.z * 255),
		255
	};

	if (type == (uint32_t)Type::Point) 
	{
		auto& pos = GetTransform()->position();
		gizmos.DrawWireSphere(m_Limit, pos, c);
		GetScene()->PushGizmosInstance(&gizmos);
	}
	else if (type == (uint32_t)Type::Spot)
	{
		auto& pos = GetTransform()->position();
		auto dir = GetTransform()->Back();
		
		constexpr float coneRange = 5;
		float inner = glm::tan(glm::radians(m_Fov * (1 - m_Blend) * 0.5f)) * coneRange;
		float outer = glm::tan(glm::radians(m_Fov * 0.5f)) * coneRange;

		gizmos.DrawSpotLight(pos, dir, inner, outer, m_Limit, c);
		GetScene()->PushGizmosInstance(&gizmos);
	}
	else
	{
		auto& pos = GetTransform()->position();
		auto dir = GetTransform()->Back();
		gizmos.DrawDirectionalLight(dir, pos, c);
		GetScene()->PushGizmosInstance(&gizmos);
	}
}

template<>
_NODISCARD DirectionalLightUniform Light::GetLightUniformAs() const
{
	DirectionalLightUniform uniform;
	uniform.lightspaces = m_Lightspaces;
	uniform.color = m_Color;
	uniform.direction = m_Direction;
	uniform.angle = 0.0f;
	uniform.intensity = m_Intensity;
	uniform.shadowOffset = m_UseShadows ? 4 : 0;
	uniform.shadowStrength = 1 - m_ShadowAttenuation;
	uniform.splitDepths = m_CascadeSplitDepths;
	return uniform;
}

template<>
_NODISCARD PointLightUniform Light::GetLightUniformAs() const
{
	PointLightUniform uniform;
	uniform.lightspace = m_Lightspaces[0];
	uniform.color = m_Color;
	uniform.position = m_Position;
	uniform.intensity = m_Intensity;
	uniform.radius = m_Radius;
	uniform.limit = m_Limit;
	uniform.shadowOffset = m_UseShadows ? 1 : 0;
	uniform.shadowStrength = 1 - m_ShadowAttenuation;
	return uniform;
}

template<>
_NODISCARD SpotLightUniform Light::GetLightUniformAs() const
{
	SpotLightUniform uniform;
	uniform.lightspace = m_Lightspaces[0];
	uniform.color = m_Color;
	uniform.position = m_Position;
	uniform.direction = m_Direction;
	uniform.intensity = m_Intensity;
	uniform.radius = m_Radius;
	uniform.limit = m_Limit;
	uniform.fov = m_Fov;
	uniform.blend = m_Blend;
	uniform.shadowOffset = m_UseShadows ? 1 : 0;
	uniform.shadowStrength = 1 - m_ShadowAttenuation;
	uniform.nearClip = m_NearClip;
	return uniform;
}

void Light::Inspect() 
{
	ImGui::PushID("##LI");
	{
		ImGui::Columns(2);
		ImGui::Text("Light Type");
		ImGui::NextColumn();
		switch (type)
		{
		case 0:
			ImGui::Text("Directional");
			break;
		case 1:
			ImGui::Text("Point");
			break;
		case 2:
			ImGui::Text("Spot");
			break;
		}
		ImGui::Columns(1);

		ImGui::ColorEdit3("Light Color", (float*)&m_Color);
		
		ImGui::Columns(2);
		ImGui::Text("Intensity");
		ImGui::NextColumn();
		ImGui::DragFloat("###Intensity", &m_Intensity, 0.01f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::Columns(1);

		ImGui::Columns(2);
		ImGui::Text("Gizmos");
		ImGui::NextColumn();
		ImGui::Checkbox("###USEGIZMOS", &useGizmos);
		ImGui::Columns(1);
		ImGui::Separator(); // -----------------------------------------------------

		if (type != /*Type::Directional*/0)
		{
			ImGui::Columns(2);
			ImGui::Text("Radius");
			ImGui::NextColumn();
			ImGui::DragFloat("###Radius", &m_Radius, 0.03f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Columns(1);

			ImGui::Columns(2);
			ImGui::Text("Limit");
			ImGui::NextColumn();
			ImGui::DragFloat("###Limit", &m_Limit, 0.01f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Columns(1);
		}

		if (type == /*Type::Spot*/2)
		{
			ImGui::Columns(2);
			ImGui::Text("FOV");
			ImGui::NextColumn();
			ImGui::DragFloat("###FOV", &m_Fov, 0.5f, 0.0f, 180, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Columns(1);

			ImGui::Columns(2);
			ImGui::Text("Blend");
			ImGui::NextColumn();
			ImGui::DragFloat("###BLEND", &m_Blend, 0.002f, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Columns(1);
		}

		ImGui::Columns(2);
		ImGui::Text("Shadow Strength");
		ImGui::NextColumn();
		ImGui::DragFloat("###ShadowStrength", &m_ShadowAttenuation, 0.01f, 0.0f, 1.0f, "%.05f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::Columns(1);
	}
	ImGui::PopID();
}

void Light::UpdateDirectionalLightCascades()
{
	Camera* camera = VIEW_CAM;

	float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];

	const float cascadeSplitLambda = 0.95f;
	const float nearClip = camera->nearClipPlane;
	const float farClip = camera->farClipPlane;
	const float clipRange = farClip - nearClip;

	float minZ = nearClip;
	float maxZ = nearClip + clipRange;

	float range = maxZ - minZ;
	float ratio = maxZ / minZ;

	// Calculate split depths based on view camera frustum
	// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
	for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
		float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
		float log = minZ * std::pow(ratio, p);
		float uniform = minZ + range * p;
		float d = cascadeSplitLambda * (log - uniform) + uniform;
		cascadeSplits[i] = (d - nearClip) / clipRange;
	}

	glm::mat4 invCam = inverse(camera->getWorldToClipMatrix());

	// Calculate orthographic projection matrix for each cascade
	float lastSplitDist = 0.0;
	for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
		float splitDist = cascadeSplits[i];

		glm::vec3 frustumCorners[8] = {
			glm::vec3(-1.0f,  1.0f, 0.0f),
			glm::vec3(1.0f,  1.0f, 0.0f),
			glm::vec3(1.0f, -1.0f, 0.0f),
			glm::vec3(-1.0f, -1.0f, 0.0f),
			glm::vec3(-1.0f,  1.0f,  1.0f),
			glm::vec3(1.0f,  1.0f,  1.0f),
			glm::vec3(1.0f, -1.0f,  1.0f),
			glm::vec3(-1.0f, -1.0f,  1.0f),
		};

		// Project frustum corners into world space
		for (uint32_t j = 0; j < 8; j++) {
			glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[j], 1.0f);
			frustumCorners[j] = invCorner / invCorner.w;
		}

		for (uint32_t j = 0; j < 4; j++) {
			glm::vec3 dist = frustumCorners[j + 4] - frustumCorners[j];
			frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
			frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
		}

		// Get frustum center
		glm::vec3 frustumCenter = glm::vec3(0.0f);
		for (uint32_t j = 0; j < 8; j++) {
			frustumCenter += frustumCorners[j];
		}
		frustumCenter /= 8.0f;

		float radius = 0.0f;
		for (uint32_t j = 0; j < 8; j++) {
			float distance = glm::length(frustumCorners[j] - frustumCenter);
			radius = glm::max(radius, distance);
		}
		radius = std::ceil(radius * 16.0f) / 16.0f;

		glm::vec3 maxExtents = glm::vec3(radius);
		glm::vec3 minExtents = -maxExtents;

		glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - glm::vec3(m_Direction) * -minExtents.z, frustumCenter, Vec3::Up);
		glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

		// Store split distance and matrix in cascade
		m_CascadeSplitDepths[i] = (nearClip + splitDist * clipRange) * -1.0f;
		m_Lightspaces[i] = lightOrthoMatrix * lightViewMatrix;

		lastSplitDist = cascadeSplits[i];
	}
}

template<>
void Scene::OnComponentAdded<Light>(Entity& entity, Light& component)
{
	m_SceneLights.emplace_back(&component);
}

template<>
void Scene::OnComponentRemoved<Light>(Entity& entity, Light& component)
{
	m_SceneLights.erase(
		std::remove_if(m_SceneLights.begin(), m_SceneLights.end(), [&component](Light* light) {
			return light->entity->id() == component.entity->id();
			}),
		m_SceneLights.end()
	);
}