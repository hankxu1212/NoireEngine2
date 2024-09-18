#include "TransformMatrixStack.hpp"

#include <iostream>

void TransformMatrixStack::Multiply(glm::mat4 mat)
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
	if (stack.empty())
	{
		std::cerr << "Stack is empty!";
		return;
	}

	currentMatrix = stack.back();
	stack.pop_back();
}
