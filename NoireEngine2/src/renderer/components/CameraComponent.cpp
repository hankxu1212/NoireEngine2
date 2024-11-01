#include "CameraComponent.hpp"

#include "renderer/scene/Scene.hpp"
#include "utils/Logger.hpp"
#include "imgui/imgui.h"
#include "renderer/scene/Entity.hpp"

#include <iostream>

void CameraComponent::Update()
{
	s_Camera->Update(*GetTransform());
}

void CameraComponent::Inspect()
{
	ImGui::PushID("CameraInspect");
	{
		ImGui::Columns(2);
		ImGui::Text("%s", "Type");
		ImGui::NextColumn();
		ImGui::Text(s_Camera->getTypeStr());
		ImGui::Columns(1);

		ImGui::Columns(2);
		ImGui::Text("Priority");
		ImGui::NextColumn();
		ImGui::DragInt("##PRIO", &priority, 1, -100, 100);
		ImGui::Columns(1);

		ImGui::Columns(2);
		ImGui::Text("Aspect Ratio");
		ImGui::NextColumn();
		ImGui::DragFloat("##AR", &s_Camera->aspectRatio, 0.05f, 0, 180, "%.2f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::Columns(1);

		ImGui::Columns(2);
		ImGui::Text("Near Clip Plane");
		ImGui::NextColumn();
		ImGui::DragFloat("##NP", &s_Camera->nearClipPlane, 0.05f, -FLT_MAX, s_Camera->farClipPlane, "%.2f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::Columns(1);

		// --------------------------------------------------------

		ImGui::Columns(2);
		ImGui::Text("%s", "Far Clip Plane");
		ImGui::NextColumn();
		ImGui::DragFloat("##FP", &s_Camera->farClipPlane, 0.05f, s_Camera->nearClipPlane, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::Columns(1);

		// --------------------------------------------------------

		ImGui::Columns(2);
		ImGui::Text("%s", "Orthographic");
		ImGui::NextColumn();
		ImGui::Checkbox("##Orthographic", &s_Camera->orthographic);
		ImGui::Columns(1);

		// --------------------------------------------------------

		if (s_Camera->orthographic) {
			ImGui::Columns(2);
			ImGui::Text("%s", "Orthographic Scale");
			ImGui::NextColumn();

			ImGui::DragFloat("##OS", &s_Camera->orthographicScale, 0.01f, 0.001f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Columns(1);
		}
		else {
			ImGui::Columns(2);
			ImGui::Text("%s", "Field Of View");
			ImGui::NextColumn();

			ImGui::DragFloat("##FV", &s_Camera->fieldOfView, 0.03f, 0.0f, 180.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Columns(1);
		}
	}
	ImGui::PopID();
}

void CameraComponent::Debug()
{
	Inspect();
}

CameraComponent::CameraComponent(int priority_) :
	priority(priority_)
{
	s_Camera = std::make_unique<Camera>();
}

template<>
void Scene::OnComponentAdded<CameraComponent>(Entity& entity, CameraComponent& component)
{
	if (component.s_Camera->getType() == Camera::Type::Debug)
		return;

	NE_DEBUG(std::format("Creating new camera rendering instance with priority {}", component.priority), Logger::YELLOW, Logger::BOLD);
	m_SceneCameras.emplace_back(&component);
}

template<>
void Scene::OnComponentRemoved<CameraComponent>(Entity& entity, CameraComponent& component)
{
	if (component.s_Camera->getType() == Camera::Type::Debug)
		return;

	NE_DEBUG(std::format("Removing camera instance with priority {}", component.priority), Logger::YELLOW);

	m_SceneCameras.erase(
		std::remove_if(m_SceneCameras.begin(), m_SceneCameras.end(), [&component](CameraComponent* cam) { 
			return cam->entity->id() == component.entity->id(); 
		}), 
		m_SceneCameras.end()
	);
}