#include "RendererComponent.hpp"

#include "renderer/scene/Scene.hpp"
#include "renderer/components/CameraComponent.hpp"
#include "renderer/object/Mesh.hpp"
#include "renderer/materials/Material.hpp"
#include "Application.hpp"

#include "imgui/imgui.h"
#include <iostream>

RendererComponent::RendererComponent(Mesh* mesh_) :
	mesh(mesh_) {
}

RendererComponent::RendererComponent(Mesh* mesh_, Material* material_) :
	mesh(mesh_), material(material_) {
}

void RendererComponent::Update()
{
}

void RendererComponent::Render(const glm::mat4& model)
{
	mesh->Update(model);

	static auto cullMode = Application::GetSpecification().Culling;

	// frustum culling
	if (cullMode == ApplicationSpecification::Culling::Frustum) 
	{
		const Camera* cullCam = GetScene()->GetCullingCam()->camera();

		assert(cullCam && "No active culling camera");
		if (!cullCam->getViewFrustum().CubeInFrustum(mesh->getAABB().min, mesh->getAABB().max))
			return;
	}

	const Camera* renderCam = GetScene()->GetRenderCam()->camera();
	GetScene()->PushObjectInstance({
		{
			renderCam->getWorldToClipMatrix() * model,
			model,
			model,
		}, // transform uniform
		0, //  first vertex
		mesh, // mesh pointer
		material // material pointer
	});
}

void RendererComponent::Inspect()
{
	ImGui::PushID("Mesh Inspect");
	{
		ImGui::Columns(2);
		ImGui::Text("%s", "Mesh Name");
		ImGui::NextColumn();
		ImGui::Text(mesh->getInfo().name.c_str());
		ImGui::Columns(1);
	}
	ImGui::PopID();
}

template<>
void Scene::OnComponentAdded<RendererComponent>(Entity& entity, RendererComponent& component)
{
}

template<>
void Scene::OnComponentRemoved<RendererComponent>(Entity& entity, RendererComponent& component)
{
}