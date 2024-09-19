#pragma once

#include <filesystem>
#include "renderer/PosNorTexVertex.hpp"
#include "backend/buffers/Buffer.hpp"
#include "core/resources/Resource.hpp"

class Mesh : Resource
{
public:
	Mesh();
	~Mesh();

	using Vertex = PosNorTexVertex;

	template<typename T>
	void LoadModel(std::filesystem::path& importPath);

	virtual std::type_index getTypeIndex() const { return typeid(Mesh); }

	const Buffer& vertexBuffer() const { return m_VertexBuffer; }

	uint32_t getVertexCount() { return numVertices; }

private:
	Buffer m_VertexBuffer;
	uint32_t numVertices;
};


template<typename T>
inline void Mesh::LoadModel(std::filesystem::path& importPath)
{
}
