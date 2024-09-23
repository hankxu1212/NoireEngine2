#include "Mesh.hpp"

#include <iostream>

#include "math/Math.hpp"
#include "core/resources/Files.hpp"
#include "renderer/scene/Entity.hpp"


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
			attributeMap.at("src").as_string().value(),
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
		obj.at("material").as_string().value()
	);
}

void Mesh::Deserialize(Entity* entity, const Scene::TValueMap& obj)
{
	if (!entity)
		throw std::runtime_error("Entity pointer is null while trying to load mesh from file.");

	const Mesh::CreateInfo meshInfo = Mesh::Deserialize(obj);
	entity->AddComponent<RendererComponent>(Mesh::Create(meshInfo).get());
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
		std::cout << "Reusing old mesh" << std::endl;
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
	std::cout << "Instantiated mesh from path: " << m_CreateInfo.attributes[0].src << std::endl;

	r_Vertex = std::make_shared<VertexInput>();
	r_Vertex->m_NativeAttributes = m_CreateInfo.attributes; // copy by value
	r_Vertex->Initialize();

	const std::string fullPath = "../scenes/examples/" + m_CreateInfo.attributes[0].src;
	std::vector<std::byte> bytes = Files::Read(fullPath);

	m_VertexBuffer = Buffer(
		bytes.size(),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	//copy data to buffer:
	Buffer::TransferToBuffer(bytes.data(), bytes.size(), m_VertexBuffer.getBuffer());

	numVertices = (uint32_t)bytes.size() / 48;
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
	return node;
}

Node& operator<<(Node& node, const Mesh::CreateInfo& info)
{
	node["name"].Set(info.name);
	node["topology"].Set(info.topology);
	node["count"].Set(info.count);
	node["attributes"].Set(info.attributes);
	node["material"].Set(info.material);
	return node;
}
