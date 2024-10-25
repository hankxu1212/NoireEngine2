#include "Light.hpp"
#include "renderer/scene/Entity.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "backend/pipeline/ObjectPipeline.hpp"

#include "imgui/imgui.h"

Light::Light(Type type)
{
	this->type = (uint32_t)type;
	useGizmos = true;
}

Light::Light(Type type, Color3 color, float intensity) :
	Light(type)
{
	memcpy(&this->color, color.value_ptr(), sizeof(Color3));
	this->intensity = intensity;
}

Light::Light(Type type, Color3 color, float intensity, float radius) :
	Light(type, color, intensity)
{
	this->radius = radius;
}

Light::Light(Type type, Color3 color, float intensity, float radius, float limit) :
	Light(type, color, intensity, radius)
{
	this->limit = limit;
}

Light::Light(Type type, Color3 color, float intensity, float radius, float fov, float blend) :
	Light(type, color, intensity, radius)
{
	this->fov = fov;
	this->blend = blend;
}

Light::Light(Type type, Color3 color, float intensity, float radius, float fov, float blend, float limit) :
	Light(type, color, intensity, radius, fov, blend)
{
	this->limit = limit;
}


void Light::Update()
{
	Transform* transform = GetTransform();
	glm::vec3 dir = transform->Back(); // always point in -z direction
	glm::vec3 pos = transform->WorldLocation();

	direction = glm::vec4(dir, 0);
	position = glm::vec4(pos, 0);

	if (useShadows) 
	{
		if (type == (uint32_t)Type::Directional)
		{
			glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(45.0f), 1.0f, zNear, zFar);
			glm::mat4 depthViewMatrix = glm::lookAt(pos, pos + dir, transform->Up());
			lightspaces[0] = depthProjectionMatrix * depthViewMatrix;
		}
		else if (type == (uint32_t)Type::Point)
		{
			// TODO: add
		}
		else  // Type::Spot
		{
			glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(fov), 1.0f, zNear, zFar);
			glm::mat4 depthViewMatrix = glm::lookAt(pos, pos + dir, transform->Up());
			lightspaces[0] = depthProjectionMatrix * depthViewMatrix;
		}
	}
}

void Light::Render(const glm::mat4& model)
{
	if (!ObjectPipeline::UseGizmos || !useGizmos)
		return;

	Color4_4 c{
		static_cast<uint8_t>(color.x * 255),
		static_cast<uint8_t>(color.y * 255),
		static_cast<uint8_t>(color.z * 255),
		255
	};

	if (type == (uint32_t)Type::Point) 
	{
		auto& pos = GetTransform()->position();
		gizmos.DrawWireSphere(limit, pos, c);
		GetScene()->PushGizmosInstance(&gizmos);
	}
	else if (type == (uint32_t)Type::Spot)
	{
		auto& pos = GetTransform()->position();
		auto dir = GetTransform()->Back();
		
		constexpr float coneRange = 5;
		float inner = glm::tan(glm::radians(fov * (1 - blend) * 0.5f)) * coneRange;
		float outer = glm::tan(glm::radians(fov * 0.5f)) * coneRange;

		gizmos.DrawSpotLight(pos, dir, inner, outer, limit, c);
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
	uniform.lightspaces = lightspaces;
	uniform.color = color;
	uniform.direction = direction;
	uniform.angle = 0.0f;
	uniform.intensity = intensity;
	uniform.shadowOffset = useShadows ? 1 : 0;
	uniform.shadowStrength = 1 - oneMinusShadowStrength;
	return uniform;
}

template<>
_NODISCARD PointLightUniform Light::GetLightUniformAs() const
{
	PointLightUniform uniform;
	uniform.lightspace = lightspaces[0];
	uniform.color = color;
	uniform.position = position;
	uniform.intensity = intensity;
	uniform.radius = radius;
	uniform.limit = limit;
	uniform.shadowOffset = useShadows ? 1 : 0;
	uniform.shadowStrength = 1 - oneMinusShadowStrength;
	return uniform;
}

template<>
_NODISCARD SpotLightUniform Light::GetLightUniformAs() const
{
	SpotLightUniform uniform;
	uniform.lightspace = lightspaces[0];
	uniform.color = color;
	uniform.position = position;
	uniform.direction = direction;
	uniform.intensity = intensity;
	uniform.radius = radius;
	uniform.limit = limit;
	uniform.fov = fov;
	uniform.blend = blend;
	uniform.shadowOffset = useShadows ? 1 : 0;
	uniform.shadowStrength = 1 - oneMinusShadowStrength;
	uniform.nearClip = zNear;
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

		ImGui::ColorEdit3("Light Color", (float*)&color);
		
		ImGui::Columns(2);
		ImGui::Text("Intensity");
		ImGui::NextColumn();
		ImGui::DragFloat("###Intensity", &intensity, 0.01f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
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
			ImGui::DragFloat("###Radius", &radius, 0.03f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Columns(1);

			ImGui::Columns(2);
			ImGui::Text("Limit");
			ImGui::NextColumn();
			ImGui::DragFloat("###Limit", &limit, 0.01f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Columns(1);
		}

		if (type == /*Type::Spot*/2)
		{
			ImGui::Columns(2);
			ImGui::Text("FOV");
			ImGui::NextColumn();
			ImGui::DragFloat("###FOV", &fov, 0.5f, 0.0f, 180, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Columns(1);

			ImGui::Columns(2);
			ImGui::Text("Blend");
			ImGui::NextColumn();
			ImGui::DragFloat("###BLEND", &blend, 0.002f, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Columns(1);
		}

		ImGui::Columns(2);
		ImGui::Text("Shadow Strength");
		ImGui::NextColumn();
		ImGui::DragFloat("###ShadowStrength", &oneMinusShadowStrength, 0.01f, 0.0f, 1.0f, "%.05f", ImGuiSliderFlags_AlwaysClamp);
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