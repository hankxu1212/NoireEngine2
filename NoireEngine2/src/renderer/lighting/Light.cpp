#include "Light.hpp"
#include "renderer/scene/Entity.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "imgui/imgui.h"

Light::Light(Type type)
{
	m_LightUniform.type = (uint32_t)type;
}

Light::Light(Type type, Color3 color, float intensity) :
	Light(type)
{
	memcpy(&m_LightUniform.color, color.value_ptr(), sizeof(Color3));
	m_LightUniform.intensity = intensity;
}

void Light::Update()
{
	Transform* transform = GetTransform();
	memcpy(&m_LightUniform.direction, glm::value_ptr(transform->Forward()), sizeof(glm::vec3));
	memcpy(&m_LightUniform.position, glm::value_ptr(transform->LocalDirty()[3]), sizeof(glm::vec3));
}

Light::Light(Type type, Color3 color, float intensity, float radius) :
	Light(type, color, intensity)
{
	m_LightUniform.radius = radius;
}

Light::Light(Type type, Color3 color, float intensity, float radius, float limit) :
	Light(type, color, intensity, radius)
{
	m_LightUniform.limit = limit;
}

Light::Light(Type type, Color3 color, float intensity, float radius, float fov, float blend) :
	Light(type, color, intensity, radius)
{
	m_LightUniform.fov = fov;
	m_LightUniform.blend = blend;
}

Light::Light(Type type, Color3 color, float intensity, float radius, float fov, float blend, float limit) :
	Light(type, color, intensity, radius, fov, blend)
{
	m_LightUniform.limit = limit;
}

void Light::Inspect() 
{
	ImGui::PushID("##LI");
	{
		ImGui::Columns(2);
		ImGui::Text("Light Type");
		ImGui::NextColumn();
		switch (m_LightUniform.type)
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

		ImGui::ColorEdit3("Light Color", (float*)&m_LightUniform.color);
		
		ImGui::Columns(2);
		ImGui::Text("Intensity");
		ImGui::NextColumn();
		ImGui::DragFloat("###Intensity", &m_LightUniform.intensity, 0.01f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::Columns(1);

		if (m_LightUniform.type != /*Type::Directional*/0)
		{
			ImGui::Columns(2);
			ImGui::Text("Radius");
			ImGui::NextColumn();
			ImGui::DragFloat("###Radius", &m_LightUniform.radius, 0.01f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Columns(1);

			ImGui::Columns(2);
			ImGui::Text("Limit");
			ImGui::NextColumn();
			ImGui::DragFloat("###Limit", &m_LightUniform.limit, 0.01f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
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