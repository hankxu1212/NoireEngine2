#pragma once

#include "renderer/vertices/Vertex.hpp"
#include "backend/buffers/Buffer.hpp"
#include "core/resources/Resources.hpp"
#include "renderer/scene/Scene.hpp"

class Mesh : public Resource
{
public:
	struct CreateInfo
	{
		std::string name;
		std::string topology;
		uint32_t count;
		std::vector<VertexInput::Attribute> attributes;
		std::string material;

		CreateInfo() = default;

		CreateInfo(const std::string& name_,
			const std::string& topology_,
			uint32_t count_,
			std::vector< VertexInput::Attribute>& attributes_,
			const std::string& material_) 
		{
			name.assign(name_);
			topology.assign(topology_);
			count = count_;
			attributes = attributes_;
			material.assign(material_);
		}
		
		CreateInfo(const CreateInfo& other)
		{
			name.assign(other.name);
			topology.assign(other.topology);
			count = other.count;
			attributes = other.attributes;
			material.assign(other.material);
		}
	};

public:
	Mesh() = default;

	Mesh(const CreateInfo& createInfo);

	~Mesh();

public:

	static const CreateInfo Deserialize(const Scene::TValueMap& obj);
	static void Deserialize(Entity* entity, const Scene::TValueMap& obj);

	static std::shared_ptr<Mesh> Create(const CreateInfo& createInfo);
	static std::shared_ptr<Mesh> Create(const Node& node);

	// loads the mesh from create info
	void Load();

	friend const Node& operator>>(const Node& node, Mesh& mesh);
	friend Node& operator<<(Node& node, const Mesh& mesh);

	friend const Node& operator>>(const Node& node, CreateInfo& mesh);
	friend Node& operator<<(Node& node, const CreateInfo& mesh);

public:
	virtual std::type_index getTypeIndex() const { return typeid(Mesh); }

	const Buffer& vertexBuffer() const { return m_VertexBuffer; }

	uint32_t getVertexCount() { return numVertices; }

	std::shared_ptr<VertexInput>	getVertexInput() { return r_Vertex; }

private:
	Buffer							m_VertexBuffer;
	uint32_t						numVertices;
	std::shared_ptr<VertexInput>	r_Vertex;
	CreateInfo						m_CreateInfo;
};