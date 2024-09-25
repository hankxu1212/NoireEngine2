#include "Mesh.hpp"

#include <iostream>

#include "math/Math.hpp"
#include "core/resources/Files.hpp"
#include "renderer/scene/Entity.hpp"
#include "utils/Logger.hpp"
#include "renderer/materials/Material.hpp"

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

	return CreateInfo (
		obj.at("name").as_string().value(),
		obj.at("topology").as_string().value(),
		(uint32_t)obj.at("count").as_number().value(),
		vertexAttributes,
		obj.at("material").as_string().value(),
		attributesMap.at("POSITION").as_object().value().at("src").as_string().value()
	);
}

void Mesh::Deserialize(Entity* entity, const Scene::TValueMap& obj, const Scene::TSceneMap& sceneMap)
{
	if (!entity)
		throw std::runtime_error("Entity pointer is null while trying to load mesh from file.");

	const Mesh::CreateInfo meshInfo = Mesh::Deserialize(obj);

	// material
	const auto& materialNodes = sceneMap.at(SceneNode::Material);
	if (materialNodes.find(meshInfo.material) == materialNodes.end())
		throw std::runtime_error(std::format("Could not find this material at {}", meshInfo.material));
	
	const auto& materialObjOpt = materialNodes.at(meshInfo.material).as_object();
	if (!materialObjOpt)
		throw std::runtime_error(std::format("Could not serialize this material at {}", meshInfo.material));

	entity->AddComponent<RendererComponent>(
		Mesh::Create(meshInfo).get(), 
		Material::Deserialize(materialObjOpt.value())
	);
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

	m_Vertex = VertexInput::Create(m_CreateInfo.attributes).get();

	const std::string fullPath = "../scenes/examples/" + m_CreateInfo.src;
	std::vector<std::byte> bytes = Files::Read(fullPath);

	m_VertexBuffer = Buffer(
		bytes.size(),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	//copy data to buffer:
	Buffer::TransferToBuffer(bytes.data(), bytes.size(), m_VertexBuffer.getBuffer());

	numVertices = (uint32_t)bytes.size() / m_CreateInfo.attributes[0].stride;
	assert(numVertices * m_CreateInfo.attributes[0].stride == bytes.size());

	uint32_t positionSize = sizeof(glm::vec3);

	// extract vertices and create AABB
	m_AABB.min = glm::vec3(std::numeric_limits<float>::infinity());
	m_AABB.max = -glm::vec3(std::numeric_limits<float>::infinity());

	for (uint32_t i = 0; i < numVertices; ++i)
	{
		glm::vec3 position;
		memcpy(glm::value_ptr(position), bytes.data() + i * m_CreateInfo.attributes[0].stride, positionSize);
		
		m_AABB.min = glm::min(m_AABB.min, position);
		m_AABB.max = glm::max(m_AABB.max, position);
	}

	NE_INFO("Created AABB: min: {}, max: {}", glm::to_string(m_AABB.min), glm::to_string(m_AABB.min));
}

void Mesh::Update(const glm::mat4& model)
{
	m_AABB.Update(model);
}

const Node& operator>>(const Node& node, Mesh& mesh) {
	node["numVertices"].Get(mesh.numVertices);
	node["createInfo"].Get(mesh.m_CreateInfo);
	return node;
}

Node& operator<<(Node& node, const Mesh& mesh) {
	node["numVertices"].Set(mesh.numVertices);
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
