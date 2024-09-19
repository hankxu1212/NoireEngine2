#pragma once

#include "math/Math.hpp"

#include "Mesh.hpp"

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


	//ObjectInstance(TransformUniform&& transform, uint32_t first, uint32_t count, Mesh* _mesh) :
	//	m_TransformUniform(transform), firstVertex(first), numVertices(count), mesh(_mesh) {
	//}
};

