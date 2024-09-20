#pragma once

#include "renderer/PosNorTexVertex.hpp"
#include "backend/buffers/Buffer.hpp"
#include "core/resources/Resources.hpp"

class Mesh : public Resource
{
public:
	Mesh(std::filesystem::path importPath);
	~Mesh();

	using Vertex = PosNorTexVertex;

	static std::shared_ptr<Mesh> Create(std::filesystem::path& importPath);

	static std::shared_ptr<Mesh> Create(const Node& node);

	virtual std::type_index getTypeIndex() const { return typeid(Mesh); }

	const Buffer& vertexBuffer() const { return m_VertexBuffer; }

	uint32_t getVertexCount() { return numVertices; }

	// loads the mesh from filename
	void Load();

	friend const Node& operator>>(const Node& node, Mesh& mesh);
	friend Node& operator<<(Node& node, const Mesh& mesh);

private:
	Buffer m_VertexBuffer;
	uint32_t numVertices;
	std::string filename;
};