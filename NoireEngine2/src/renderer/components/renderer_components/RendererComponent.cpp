#include "RendererComponent.hpp"

#include "renderer/object/ObjectInstance.hpp"
#include "renderer/scene/Scene.hpp"
#include "renderer/Camera.hpp"
#include "renderer/components/CameraComponent.hpp"
#include "renderer/object/Mesh.hpp"
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
	Camera* cam = GetScene()->mainCam()->camera();
	mesh->Update(model);

	// frustum culling
	if (!cam->getViewFrustum().CubeInFrustum(mesh->getAABB().min, mesh->getAABB().max))
		return;

	GetScene()->PushObjectInstances({
		{
			cam->getProjectionMatrix() * cam->getViewMatrix() * model,
			model,
			model,
		}, // transform uniform
		0, mesh->getVertexCount(), //  first vertex, num vertices
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