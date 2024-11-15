#pragma once

#include "math/Math.hpp"

class Mesh;
class Material;
class CommandBuffer;

struct ObjectInstance
{
	struct TransformUniform 
	{
		glm::mat4 modelToClip;
		glm::mat4 modelMatrix;
		glm::mat4 modelMatrix_Normal;
	}m_TransformUniform;
	static_assert(sizeof(TransformUniform) == 64 * 3, "Transform Uniform is the expected size.");

	uint32_t firstVertex = 0;

	Mesh* mesh;
	Material* material;
	uint64_t entityID;

	ObjectInstance(
		const glm::mat4& modelToClip,
		const glm::mat4& model,
		const glm::mat4& normal,
		uint32_t vertexIndex,
		Mesh* meshPtr,
		Material* materialPtr,
		uint64_t eid
	) : 
		m_TransformUniform{ modelToClip, model, normal },
		firstVertex(vertexIndex),
		mesh(meshPtr),
		material(materialPtr),
		entityID(eid)
	{}

	// Copy Constructor
	ObjectInstance(const ObjectInstance& other)
		: m_TransformUniform(other.m_TransformUniform),
		firstVertex(other.firstVertex),
		mesh(other.mesh),
		material(other.material),
		entityID(other.entityID)
	{
	}

	// Move Constructor
	ObjectInstance(ObjectInstance&& other) noexcept
		: m_TransformUniform(std::move(other.m_TransformUniform)),
		firstVertex(other.firstVertex),
		mesh(other.mesh),
		material(other.material),
		entityID(other.entityID)
	{
		// Set the moved-from object's pointers to nullptr
		other.mesh = nullptr;
		other.material = nullptr;
	}
};