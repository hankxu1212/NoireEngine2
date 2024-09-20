#pragma once

#include "renderer/vertices/PosNorTanTexVertex.hpp"
#include "backend/buffers/Buffer.hpp"
#include "core/resources/Resources.hpp"
#include "renderer/scene/Scene.hpp"

class Mesh : public Resource
{
public:
	using Vertex = PosNorTanTexVertex;

public:
	Mesh(std::filesystem::path importPath);
	~Mesh();

public:
	struct Attribute
	{
		const std::string& src;
		uint32_t offset;
		uint32_t stride;
		const std::string& format;
	};
	static_assert(sizeof(Attribute) == 8 + 4 + 4 + 8);

	struct Object
	{
		const std::string& name;
		const std::string& topology;
		uint32_t count;
		Attribute attributes[4];
		const std::string& material;
	};
	static_assert(sizeof(Object) == 8 + 8 + (4 + 4) + 4 * 24 + 8);

	static Object Deserialize(const Scene::TValueMap& obj);
	static void Deserialize(Entity* entity, const Scene::TValueMap& obj);

	static std::shared_ptr<Mesh> Create(std::filesystem::path& importPath);

	static std::shared_ptr<Mesh> Create(const std::string& importPath);

	static std::shared_ptr<Mesh> Create(const Node& node);

	// loads the mesh from filename
	void Load();

	friend const Node& operator>>(const Node& node, Mesh& mesh);

	friend Node& operator<<(Node& node, const Mesh& mesh);

public:
	virtual std::type_index getTypeIndex() const { return typeid(Mesh); }

	const Buffer& vertexBuffer() const { return m_VertexBuffer; }

	uint32_t getVertexCount() { return numVertices; }

private:
	Buffer m_VertexBuffer;
	uint32_t numVertices;
	std::string filename;
};