#include "Entity.hpp"

#include "TransformMatrixStack.hpp"
#include "Scene.hpp"
#include "renderer/object/ObjectInstance.hpp"

#include <iostream>

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
}
