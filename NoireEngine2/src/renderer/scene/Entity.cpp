#include "Entity.hpp"
#include "TransformMatrixStack.hpp"

#include <iostream>

Entity::Entity() :
	Entity("Entity") {
}

Entity::Entity(glm::vec3& position) :
	s_Transform(std::make_unique<Transform>(position)) {
}

Entity::Entity(const char* name) :
	s_Transform(std::make_unique<Transform>()), m_Name(name) {
}

Entity::Entity(const Entity&)
{
}

Entity::~Entity()
{
}

bool areMatricesEqual(const glm::mat4& mat1, const glm::mat4& mat2, float epsilon = 1e-6f) {
	// Compare each element of the matrices
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			if (std::fabs(mat1[i][j] - mat2[i][j]) > epsilon) {
				return false; // Matrices are not equal
			}
		}
	}
	return true; // Matrices are equal
}

void Entity::Update()
{
	// update components
	for (auto& component : m_Components)
	{
		component.Update();
	}

	// traverse children in recursive manner
	for (auto& child : m_Children)
	{
		child->Update();
	}
}

void Entity::RenderPass(TransformMatrixStack& matrixStack)
{
	matrixStack.Multiply(s_Transform->Local());

	for (auto& child : m_Children)
	{
		glm::mat4 beforePush = matrixStack.Peek();
		
		matrixStack.Push();
		child->RenderPass(matrixStack);
		matrixStack.Pop();

		glm::mat4 afterPush = matrixStack.Peek();

		assert(areMatricesEqual(beforePush, afterPush));
	}

	// Add render code here:
	std::cout << "Rendered: " << m_Id << std::endl;
}
