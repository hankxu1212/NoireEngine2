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
	memcpy(&m_Info.direction, glm::value_ptr(transform->Forward()), sizeof(glm::vec3));
	memcpy(&m_Info.position, glm::value_ptr(transform->LocalDirty()[3]), sizeof(glm::vec3));
}

void Light::Render(const glm::mat4& model)
{
	if (!ObjectPipeline::UseGizmos || !useGizmos)
		return;

	if (m_Info.type == (uint32_t)Type::Point) 
	{
		auto& pos = GetTransform()->position();
		Color4_4 c{ 
			static_cast<uint8_t>(m_Info.color.x * 255), 
			static_cast<uint8_t>(m_Info.color.y * 255), 
			static_cast<uint8_t>(m_Info.color.z * 255), 
			255 
		};
		gizmos.DrawWireSphere(m_Info.limit, pos, c);
		GetScene()->PushGizmosInstance(&gizmos);
	}
	else if (m_Info.type == (uint32_t)Type::Spot)
	{
		auto& pos = GetTransform()->position();
		auto dir = GetTransform()->Back();
		Color4_4 c{
			static_cast<uint8_t>(m_Info.color.x * 255),
			static_cast<uint8_t>(m_Info.color.y * 255),
			static_cast<uint8_t>(m_Info.color.z * 255),
			255
		};
		gizmos.DrawSpotLight(pos, dir, m_Info.fov * (1 - m_Info.blend), m_Info.fov, m_Info.limit, c);
		GetScene()->PushGizmosInstance(&gizmos);
	}
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
			ImGui::DragFloat("###FOV", &m_Info.fov, 0.01f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
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