#include "RendererComponent.hpp"

#include "renderer/object/ObjectInstance.hpp"
#include "renderer/scene/Scene.hpp"
#include "renderer/Camera.hpp"
#include "renderer/components/CameraComponent.hpp"

RendererComponent::RendererComponent(Mesh* mesh_) :
	mesh(mesh_)
{
	
}

void RendererComponent::Update()
{
}

void RendererComponent::Render(const glm::mat4& model)
{
	Camera* cam = GetScene()->mainCam()->camera();

	ObjectInstance instance
	{
		.m_TransformUniform
		{
			.viewMatrix = cam->getProjectionMatrix() * cam->getViewMatrix() * model,
			.modelMatrix = model,
			.modelMatrix_Normal = model,
		},
		.firstVertex = 0,
		.numVertices = mesh->getVertexCount(),
		.mesh = mesh
	};

	GetScene()->PushObjectInstances(std::move(instance));
}

template<>
void Scene::OnComponentAdded<RendererComponent>(Entity& entity, RendererComponent& component)
{
}

template<>
void Scene::OnComponentRemoved<RendererComponent>(Entity& entity, RendererComponent& component)
{
}