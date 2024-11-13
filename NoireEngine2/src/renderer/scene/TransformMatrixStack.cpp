#include "TransformMatrixStack.hpp"

#include <iostream>

void TransformMatrixStack::Multiply(const glm::mat4& mat)
{
	currentMatrix *= mat; // does right matrix multiplication
}

const glm::mat4& TransformMatrixStack::Peek() const
{
	return currentMatrix;
}

void TransformMatrixStack::Clear()
{
	stack.clear();
	currentMatrix = Mat4::Identity;
}

void TransformMatrixStack::Push()
{
	stack.emplace_back(currentMatrix);
}

void TransformMatrixStack::Pop()
{
	assert(!stack.empty() && "Stack is empty");
	currentMatrix = stack.back();
	stack.pop_back();
}
