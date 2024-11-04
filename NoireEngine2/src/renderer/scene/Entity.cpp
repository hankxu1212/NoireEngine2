#include "Entity.hpp"

#include "TransformMatrixStack.hpp"
#include "Scene.hpp"
#include "renderer/object/ObjectInstance.hpp"

#include "glm/gtx/string_cast.hpp"
#include "utils/Logger.hpp"

#include <iostream>

Entity::~Entity()
{
	m_Components.clear();
}

void Entity::SetParent(Entity* newParent)
{
	m_Parent = newParent;
	s_Transform->SetParent(newParent->transform());
}

void Entity::Update()
{
	if (s_Transform && s_Transform->parent() != nullptr && s_Transform->parent()->isDirty)
		s_Transform->isDirty = true;

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

void Entity::RenderPass(TransformMatrixStack& matrixStack)
{
	matrixStack.Push();
	{
		matrixStack.Multiply(std::move(s_Transform->LocalDirty()));

		// render self
		const glm::mat4& model = matrixStack.Peek();
		for (auto& component : m_Components)
		{
			component->Render(model);
		}

		// render children first
		for (auto& child : m_Children)
		{
			child->RenderPass(matrixStack);
		}
	}
	matrixStack.Pop();
}

void Entity::PrepareAcceleration(TransformMatrixStack& matrixStack)
{
	matrixStack.Push();
	{
		matrixStack.Multiply(std::move(s_Transform->LocalDirty()));

		// render self
		const glm::mat4& model = matrixStack.Peek();
		RendererComponent* renderer = GetComponent<RendererComponent>();
		if (renderer)
			renderer->PrepareAcceleration(model);

		// render children first
		for (auto& child : m_Children)
		{
			child->PrepareAcceleration(matrixStack);
		}
	}
	matrixStack.Pop();
}
