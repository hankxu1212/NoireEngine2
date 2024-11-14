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

	Mesh* mesh;
	Material* material;
	uint64_t entityID;

	ObjectInstance(
		const glm::mat4& view,
		const glm::mat4& model,
		const glm::mat4& normal,
		uint32_t vertexIndex,
		Mesh* meshPtr,
		Material* materialPtr,
		uint64_t eid
	) : 
		m_TransformUniform{ view, model, normal },
		firstVertex(vertexIndex),
		mesh(meshPtr),
		material(materialPtr),
		entityID(eid)
	{}
};