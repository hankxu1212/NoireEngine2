#include "Light.hpp"
#include "renderer/scene/Entity.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "imgui/imgui.h"

Light::Light(Type type)
{
	m_LightUniform.type = (uint32_t)type;
}

Light::Light(Type type, Color3 color, float intensity)
{
	m_LightUniform.type = (uint32_t)type;
	memcpy(&m_LightUniform.color, color.value_ptr(), sizeof(Color3));
	m_LightUniform.intensity = intensity;
}

void Light::Update()
{
	Transform* transform = GetTransform();
	memcpy(&m_LightUniform.direction, glm::value_ptr(transform->Forward()), sizeof(glm::vec3));
	memcpy(&m_LightUniform.position, glm::value_ptr(transform->LocalDirty()[3]), sizeof(glm::vec3));
}

void Light::Inspect() 
{
	ImGui::PushID("##LI");
	{
		ImGui::ColorEdit3("Light Color", (float*)&m_LightUniform.color);
		
		ImGui::Columns(2);
		ImGui::Text("%s", "Intensity");
		ImGui::NextColumn();
		ImGui::DragFloat("###Intensitie", &m_LightUniform.intensity, 0.01f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::Columns(1);
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