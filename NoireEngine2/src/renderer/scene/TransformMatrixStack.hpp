#pragma once

#include "math/Math.hpp"
#include <vector>

class TransformMatrixStack
{
public:
	void Multiply(glm::mat4 mat);
	void Push();
	void Pop();
	const glm::mat4& Peek() const;
	void Clear();

	size_t Size() { return stack.size(); }

private:
	glm::mat4 currentMatrix = Mat4::Identity;
	std::vector<glm::mat4> stack;
};

