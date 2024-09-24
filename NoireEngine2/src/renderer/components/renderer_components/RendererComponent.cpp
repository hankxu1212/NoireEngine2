#include "RendererComponent.hpp"

#include "renderer/object/ObjectInstance.hpp"
#include "renderer/scene/Scene.hpp"
#include "renderer/Camera.hpp"
#include "renderer/components/CameraComponent.hpp"

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

template<>
void Scene::OnComponentAdded<RendererComponent>(Entity& entity, RendererComponent& component)
{
}

template<>
void Scene::OnComponentRemoved<RendererComponent>(Entity& entity, RendererComponent& component)
{
}