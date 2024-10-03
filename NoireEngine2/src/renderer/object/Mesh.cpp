#include "Mesh.hpp"

#include <iostream>

#include "math/Math.hpp"
#include "core/resources/Files.hpp"
#include "renderer/scene/Entity.hpp"
#include "utils/Logger.hpp"
#include "renderer/materials/Material.hpp"
#include "core/Core.hpp"
#include "core/Timer.hpp"

#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtx/extended_min_max.hpp"

Mesh::Mesh(const CreateInfo& createInfo) :
	m_CreateInfo(createInfo)
{
}

Mesh::~Mesh()
{
	m_VertexBuffer.Destroy();
	m_IndexBuffer.Destroy();
}

const Mesh::CreateInfo Mesh::Deserialize(const Scene::TValueMap& obj)
{
	const auto& attributesMap = obj.at("attributes").as_object().value();

	auto GET_ATTRIBUTE = [&attributesMap](const char* key) {
		const auto& attributeMap = attributesMap.at(key).as_object().value();
		return VertexInput::Attribute
		{
			(uint32_t)attributeMap.at("offset").as_number().value(),
			(uint32_t)attributeMap.at("stride").as_number().value(),
			attributeMap.at("format").as_string().value(),
		};
	};

	std::vector<VertexInput::Attribute> vertexAttributes{
		GET_ATTRIBUTE("POSITION"),
		GET_ATTRIBUTE("NORMAL"),
		GET_ATTRIBUTE("TANGENT"),
		GET_ATTRIBUTE("TEXCOORD")
	};

	std::string materialStr;
	if (obj.find("material") == obj.end() || !obj.at("material").as_string())
		materialStr = NE_NULL_STR;
	else
		materialStr = obj.at("material").as_string().value();

	return CreateInfo (
		obj.at("name").as_string().value(),
		obj.at("topology").as_string().value(),
		(uint32_t)obj.at("count").as_number().value(),
		vertexAttributes,
		materialStr,
		attributesMap.at("POSITION").as_object().value().at("src").as_string().value()
	);
}

void Mesh::Deserialize(Entity* entity, const Scene::TValueMap& obj, const Scene::TSceneMap& sceneMap)
{
	if (!entity)
		NE_ERROR("Entity pointer is null while trying to load mesh from file.");

	const Mesh::CreateInfo meshInfo = Mesh::Deserialize(obj);

	// material
	Material* mat = nullptr;
	if (meshInfo.material == NE_NULL_STR) 
	{
		mat = Material::CreateDefault().get();
		NE_INFO("No material found on this mesh, using default lambertian...");
	}
	else {
		const auto& materialNodes = sceneMap.at(SceneNode::Material);
		auto matIt = materialNodes.find(meshInfo.material);
		if (matIt == materialNodes.end())
			NE_ERROR("Could not find this material at {}", meshInfo.material);

		const auto& materialObjOpt = matIt->second.as_object();
		if (!materialObjOpt)
			NE_ERROR("Could not serialize this material at {}", meshInfo.material);

		mat = Material::Deserialize(materialObjOpt.value());
	}

	entity->AddComponent<RendererComponent>(Mesh::Create(meshInfo).get(), mat);
}

std::shared_ptr<Mesh> Mesh::Create(const CreateInfo& createInfo)
{
	Mesh temp(createInfo);
	Node node;
	node << temp;
	return Create(node);
}

std::shared_ptr<Mesh> Mesh::Create(const Node& node)
{
	if (auto resource = Resources::Get()->Find<Mesh>(node)) {
		NE_INFO("Reusing old mesh from {}", resource->m_CreateInfo.src);
		return resource;
	}

	auto result = std::make_shared<Mesh>();
	Resources::Get()->Add(node, std::dynamic_pointer_cast<Resource>(result));
	node >> *result;
	result->Load();
	return result;
}

void Mesh::Load()
{
	NE_INFO("Instantiated mesh from {}", m_CreateInfo.src);

	// create vertex input
	m_Vertex = VertexInput::Create(m_CreateInfo.attributes).get();

	// load all bytes from binary
	const std::string fullPath = "../scenes/examples/" + m_CreateInfo.src;
	std::vector<std::byte> bytes = Files::Read(fullPath);

	// extract vertex information
	// ASSUMING: POSITION, NORMAL, TANGENT, TEXCOORD  (PosNorTanTexVertex)
	// ASSUMING: every stride is the same, and come from the same src
	// TODO: add more possible vertex configurations
	Vertex* vertices = (Vertex*)bytes.data();
	TransformToIndexedMesh(vertices, m_CreateInfo.count);
}

void Mesh::CreateAABB(const std::vector<Vertex>& vertices)
{
	m_AABB.min = glm::vec3(std::numeric_limits<float>::infinity());
	m_AABB.max = -glm::vec3(std::numeric_limits<float>::infinity());

	for (uint32_t i = 0; i < vertices.size(); ++i)
	{
		m_AABB.min = glm::min(m_AABB.min, vertices[i].position);
		m_AABB.max = glm::max(m_AABB.max, vertices[i].position);
	}
}

void Mesh::TransformToIndexedMesh(Vertex* vertices, uint32_t count)
{
	Timer timer;

	std::unordered_map<Vertex, uint32_t> vertexToIndexMap;
	
	std::vector<uint32_t> indices;
	indices.reserve(m_CreateInfo.count);

	std::vector<Vertex> uniqueVertices;
	uniqueVertices.reserve(m_CreateInfo.count / 3);

	for (uint32_t i = 0; i < count; ++i)
	{
		Vertex& vertex = *&vertices[i];

		if (vertexToIndexMap.find(vertex) != vertexToIndexMap.end()) {
			indices.emplace_back(vertexToIndexMap[vertex]);
		}
		else
		{
			uniqueVertices.push_back(vertex);
			uint32_t newIndex = static_cast<uint32_t>(uniqueVertices.size()) - 1;
			indices.push_back(newIndex);
			vertexToIndexMap[vertex] = newIndex;
		}
	}

	numVertices = (uint32_t)uniqueVertices.size();
	numIndices = (uint32_t)indices.size();

	CreateAABB(uniqueVertices);
	CreateVertexBuffer(uniqueVertices);
	CreateIndexBuffer(indices);

	NE_INFO("Loading as indexed mesh took {}ms", timer.GetElapsed(true));
}

void Mesh::CreateVertexBuffer(std::vector<Vertex>& vertices)
{
	m_VertexBuffer = Buffer(
		vertices.size() * 48,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	Buffer::TransferToBuffer(vertices.data(), vertices.size() * 48, m_VertexBuffer.getBuffer());
}

void Mesh::CreateIndexBuffer(std::vector<uint32_t> indices)
{
	m_IndexBuffer = Buffer(
		indices.size() * 4,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	Buffer::TransferToBuffer(indices.data(), indices.size() * 4, m_IndexBuffer.getBuffer());
}

void Mesh::Update(const glm::mat4& model)
{
	m_AABB.Update(model);
}

void Mesh::Bind(const CommandBuffer& commandBuffer)
{
	std::array< VkBuffer, 1 > vertex_buffers{ m_VertexBuffer.getBuffer() };
	std::array< VkDeviceSize, 1 > offsets{ 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0,1, vertex_buffers.data(), offsets.data());

	vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

const Node& operator>>(const Node& node, Mesh& mesh) {
	node["createInfo"].Get(mesh.m_CreateInfo);
	return node;
}

Node& operator<<(Node& node, const Mesh& mesh) {
	node["createInfo"].Set(mesh.m_CreateInfo);
	return node;
}

const Node& operator>>(const Node& node, Mesh::CreateInfo& info)
{
	node["name"].Get(info.name);
	node["topology"].Get(info.topology);
	node["count"].Get(info.count);
	node["attributes"].Get(info.attributes);
	node["material"].Get(info.material);
	node["src"].Get(info.src);
	return node;
}

Node& operator<<(Node& node, const Mesh::CreateInfo& info)
{
	node["name"].Set(info.name);
	node["topology"].Set(info.topology);
	node["count"].Set(info.count);
	node["attributes"].Set(info.attributes);
	node["material"].Set(info.material);
	node["src"].Set(info.src);
	return node;
}
