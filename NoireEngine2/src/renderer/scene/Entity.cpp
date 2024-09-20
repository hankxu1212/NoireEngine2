#include "Entity.hpp"

#include "TransformMatrixStack.hpp"
#include "Scene.hpp"
#include "renderer/object/ObjectInstance.hpp"

#include <iostream>

Entity::Entity(Scene* scene) :
	Entity(scene, "Entity") {
}

Entity::Entity(Scene* scene, glm::vec3& position) :
	m_Scene(scene), s_Transform(std::make_unique<Transform>(position)) {
}

Entity::Entity(Scene* scene, glm::vec3& position, glm::quat& rotation, glm::vec3& scale) :
	m_Scene(scene), s_Transform(std::make_unique<Transform>(position, rotation, scale)) {
}

Entity::Entity(Scene* scene, const char* name) :
	m_Scene(scene), s_Transform(std::make_unique<Transform>()), m_Name(name) {
}

Entity::Entity(const Entity&)
{
}

Entity::~Entity()
{
	m_Components.clear();
}

void Entity::Update()
{
	// update components
	for (auto& component : m_Components)
	{
		component->Update();
	}

	// traverse children in recursive manner
	for (auto& child : m_Children)
	{
		child->Update();
	}
}

#include "core/Time.hpp"

void Entity::RenderPass(TransformMatrixStack& matrixStack)
{
	//matrixStack.Multiply(s_Transform->Local());

	for (auto& child : m_Children)
	{
		//matrixStack.Push();
		child->RenderPass(matrixStack);
		//matrixStack.Pop();
	}

	//glm::mat4 model = matrixStack.Peek();


	const glm::mat4 model = s_Transform->World();
	for (auto& component : m_Components)
	{
		component->Render(model);
	}

//	if (!m_Components.empty())
//		return;
//
//	Camera* cam = m_Scene->mainCam()->camera();
//	
//	static std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();
//
//	ObjectInstance instance
//	{
//		.m_TransformUniform
//		{
//			.viewMatrix = cam->getProjectionMatrix() * cam->getViewMatrix() * model,
//			.modelMatrix = model,
//			.modelMatrix_Normal = model,
//		},
//		.firstVertex = 0,
//		.numVertices = mesh->getVertexCount(),
//		.mesh = mesh.get()
//	};
//
//	m_Scene->PushObjectInstances(std::move(instance));
}
