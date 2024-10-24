#include "Light.hpp"
#include "renderer/scene/Entity.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "backend/pipeline/ObjectPipeline.hpp"

#include "imgui/imgui.h"

Light::Light(Type type)
{
	m_Info.type = (uint32_t)type;
}

Light::Light(Type type, Color3 color, float intensity) :
	Light(type)
{
	memcpy(&m_Info.color, color.value_ptr(), sizeof(Color3));
	m_Info.intensity = intensity;
}

void Light::Update()
{
	Transform* transform = GetTransform();
	glm::vec3 dir = transform->Forward();
	glm::vec3 pos = transform->WorldLocation();

	m_Info.direction = glm::vec4(dir, 0);
	m_Info.position = glm::vec4(pos, 0);

	if (m_Info.useShadows) 
	{
		if (m_Info.type == (uint32_t)Type::Directional)
		{
			glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(45.0f), 1.0f, m_Info.zNear, m_Info.zFar);
			glm::mat4 depthViewMatrix = glm::lookAt(pos, pos - dir, Vec3::Up);
			m_Info.lightspace = depthProjectionMatrix * depthViewMatrix;
		}
		else if (m_Info.type == (uint32_t)Type::Point)
		{
			// TODO: add
		}
		else {
			glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(m_Info.fov), 1.0f, m_Info.zNear, m_Info.zFar);
			glm::mat4 depthViewMatrix = glm::lookAt(pos, pos - dir, Vec3::Up);
			m_Info.lightspace = depthProjectionMatrix * depthViewMatrix;
		}
	}
}

void Light::Render(const glm::mat4& model)
{
	if (!ObjectPipeline::UseGizmos || !useGizmos)
		return;

	Color4_4 c{
		static_cast<uint8_t>(m_Info.color.x * 255),
		static_cast<uint8_t>(m_Info.color.y * 255),
		static_cast<uint8_t>(m_Info.color.z * 255),
		255
	};

	if (m_Info.type == (uint32_t)Type::Point) 
	{
		auto& pos = GetTransform()->position();
		gizmos.DrawWireSphere(m_Info.limit, pos, c);
		GetScene()->PushGizmosInstance(&gizmos);
	}
	else if (m_Info.type == (uint32_t)Type::Spot)
	{
		auto& pos = GetTransform()->position();
		auto dir = GetTransform()->Back();
		
		float inner = glm::tan(glm::radians(m_Info.fov * (1 - m_Info.blend) * 0.5f)) * m_Info.limit;
		float outer = glm::tan(glm::radians(m_Info.fov * 0.5f)) * m_Info.limit;

		gizmos.DrawSpotLight(pos, dir, inner, outer, m_Info.limit, c);
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
	uniform.lightspace = m_Info.lightspace;
	uniform.color = m_Info.color;
	uniform.direction = m_Info.direction;
	uniform.angle = 0.0f;
	uniform.intensity = m_Info.intensity;
	uniform.shadowOffset = m_Info.useShadows ? 1 : 0;
	return uniform;
}

template<>
_NODISCARD PointLightUniform Light::GetLightUniformAs() const
{
	PointLightUniform uniform;
	uniform.lightspace = m_Info.lightspace;
	uniform.color = m_Info.color;
	uniform.position = m_Info.position;
	uniform.intensity = m_Info.intensity;
	uniform.radius = m_Info.radius;
	uniform.limit = m_Info.limit;
	uniform.shadowOffset = 0;
	return uniform;
}

template<>
_NODISCARD SpotLightUniform Light::GetLightUniformAs() const
{
	SpotLightUniform uniform;
	uniform.lightspace = m_Info.lightspace;
	uniform.color = m_Info.color;
	uniform.position = m_Info.position;
	uniform.direction = m_Info.direction;
	uniform.intensity = m_Info.intensity;
	uniform.radius = m_Info.radius;
	uniform.limit = m_Info.limit;
	uniform.fov = m_Info.fov;
	uniform.blend = m_Info.blend;
	uniform.shadowOffset = m_Info.useShadows ? 1 : 0;
	return uniform;
}

Light::Light(Type type, Color3 color, float intensity, float radius) :
	Light(type, color, intensity)
{
	m_Info.radius = radius;
}

Light::Light(Type type, Color3 color, float intensity, float radius, float limit) :
	Light(type, color, intensity, radius)
{
	m_Info.limit = limit;
}

Light::Light(Type type, Color3 color, float intensity, float radius, float fov, float blend) :
	Light(type, color, intensity, radius)
{
	m_Info.fov = fov;
	m_Info.blend = blend;
}

Light::Light(Type type, Color3 color, float intensity, float radius, float fov, float blend, float limit) :
	Light(type, color, intensity, radius, fov, blend)
{
	m_Info.limit = limit;
}

void Light::Inspect() 
{
	ImGui::PushID("##LI");
	{
		ImGui::Columns(2);
		ImGui::Text("Light Type");
		ImGui::NextColumn();
		switch (m_Info.type)
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

		ImGui::ColorEdit3("Light Color", (float*)&m_Info.color);
		
		ImGui::Columns(2);
		ImGui::Text("Intensity");
		ImGui::NextColumn();
		ImGui::DragFloat("###Intensity", &m_Info.intensity, 0.01f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::Columns(1);

		ImGui::Columns(2);
		ImGui::Text("Gizmos");
		ImGui::NextColumn();
		ImGui::Checkbox("###USEGIZMOS", &useGizmos);
		ImGui::Columns(1);
		ImGui::Separator(); // -----------------------------------------------------

		if (m_Info.type != /*Type::Directional*/0)
		{
			ImGui::Columns(2);
			ImGui::Text("Radius");
			ImGui::NextColumn();
			ImGui::DragFloat("###Radius", &m_Info.radius, 0.01f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Columns(1);

			ImGui::Columns(2);
			ImGui::Text("Limit");
			ImGui::NextColumn();
			ImGui::DragFloat("###Limit", &m_Info.limit, 0.01f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Columns(1);
		}

		if (m_Info.type == /*Type::Spot*/2)
		{
			ImGui::Columns(2);
			ImGui::Text("FOV");
			ImGui::NextColumn();
			ImGui::DragFloat("###FOV", &m_Info.fov, 0.5f, 0.0f, 180, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Columns(1);

			ImGui::Columns(2);
			ImGui::Text("Blend");
			ImGui::NextColumn();
			ImGui::DragFloat("###BLEND", &m_Info.blend, 0.002f, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Columns(1);
		}
	}
	ImGui::PopID();
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