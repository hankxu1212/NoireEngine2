#pragma once

#include "math/Math.hpp"

class Mesh;
class Material;
class CommandBuffer;

struct ObjectInstance
{
	struct TransformUniform 
	{
		glm::mat4 viewMatrix;
		glm::mat4 modelMatrix;
		glm::mat4 modelMatrix_Normal;
	}m_TransformUniform;
	static_assert(sizeof(TransformUniform) == 64 * 3, "Transform Uniform is the expected size.");

	uint32_t firstVertex = 0;
	uint32_t numVertices = 0;

	Mesh* mesh;
	Material* material;

	bool BindMesh(const CommandBuffer& commandBuffer, uint32_t instanceID) const;

	void BindVertexInput(const CommandBuffer& commandBuffer) const;

	void Draw(const CommandBuffer& commandBuffer, uint32_t instanceID) const;
};

