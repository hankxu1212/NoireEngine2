#pragma once

#include "renderer/vertices/Vertex.hpp"
#include "backend/buffers/Buffer.hpp"
#include "core/resources/Resources.hpp"
#include "renderer/scene/Scene.hpp"
#include "renderer/AABB.hpp"

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
		std::string src;

		CreateInfo() = default;

		CreateInfo(const std::string& name_,
			const std::string& topology_,
			uint32_t count_,
			std::vector< VertexInput::Attribute>& attributes_,
			const std::string& material_,
			const std::string& src_) 
		{
			name.assign(name_);
			topology.assign(topology_);
			count = count_;
			attributes = attributes_;
			material.assign(material_);
			src.assign(src_);
		}
		
		CreateInfo(const CreateInfo& other)
		{
			name.assign(other.name);
			topology.assign(other.topology);
			count = other.count;
			attributes = other.attributes;
			material.assign(other.material);
			src.assign(other.src);
		}
	};

public:
	Mesh() = default;

	Mesh(const CreateInfo& createInfo);

	~Mesh();

public: // loading and initialization
	static const CreateInfo Deserialize(const Scene::TValueMap& obj);
	static void Deserialize(Entity* entity, const Scene::TValueMap& obj, const Scene::TSceneMap& sceneMap);

	static std::shared_ptr<Mesh> Create(const CreateInfo& createInfo);
	static std::shared_ptr<Mesh> Create(const Node& node);

	// loads the mesh from create info
	void Load();

	friend const Node& operator>>(const Node& node, Mesh& mesh);
	friend Node& operator<<(Node& node, const Mesh& mesh);

	friend const Node& operator>>(const Node& node, CreateInfo& info);
	friend Node& operator<<(Node& node, const CreateInfo& info);

public:
	void Update(const glm::mat4& model);

	virtual std::type_index getTypeIndex() const { return typeid(Mesh); }

	inline const Buffer& vertexBuffer() const { return m_VertexBuffer; }

	inline uint32_t getVertexCount() const { return numVertices; }

	inline VertexInput* getVertexInput() { return m_Vertex; }

	inline const AABB& getAABB() const { return m_AABB; }

	inline const CreateInfo& getInfo() const { return m_CreateInfo; }

	void Bind(const CommandBuffer& commandBuffer);

private:
	Buffer							m_VertexBuffer;
	uint32_t						numVertices;
	VertexInput*					m_Vertex;
	CreateInfo						m_CreateInfo;
	AABB							m_AABB;
};