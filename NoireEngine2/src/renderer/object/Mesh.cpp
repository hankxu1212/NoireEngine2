#include "Mesh.hpp"

#include <iostream>

#include "math/Math.hpp"
#include "core/resources/Files.hpp"
#include "renderer/scene/Entity.hpp"

Mesh::Mesh(std::filesystem::path importPath) :
	filename(importPath.string()) {
}

Mesh::~Mesh()
{
	m_VertexBuffer.Destroy();
}

Mesh::Object Mesh::Deserialize(const Scene::TValueMap& obj)
{
	const auto& attributesMap = obj.at("attributes").as_object().value();

	auto GET_ATTRIBUTE = [&attributesMap](const char* key) {
		const auto& attributeMap = attributesMap.at(key).as_object().value();
		return Mesh::Attribute
		{
			attributeMap.at("src").as_string().value(),
			(uint32_t)attributeMap.at("offset").as_number().value(),
			(uint32_t)attributeMap.at("stride").as_number().value(),
			attributeMap.at("format").as_string().value(),
		};
		};

	return {
		obj.at("name").as_string().value(),
		obj.at("topology").as_string().value(),
		(uint32_t)obj.at("count").as_number().value(),
		{
			GET_ATTRIBUTE("POSITION"),
			GET_ATTRIBUTE("NORMAL"),
			GET_ATTRIBUTE("TANGENT"),
			GET_ATTRIBUTE("TEXCOORD")
		},
		obj.at("material").as_string().value()
	};
}

void Mesh::Deserialize(Entity* entity, const Scene::TValueMap& obj)
{
	if (!entity)
		throw std::runtime_error("Entity pointer is null while trying to load mesh from file.");

	Mesh::Object meshObject = Mesh::Deserialize(obj);
	entity->AddComponent<RendererComponent>(Mesh::Create(meshObject.attributes[0].src).get());
}

std::shared_ptr<Mesh> Mesh::Create(std::filesystem::path& importPath)
{
	Mesh temp(importPath);
	Node node;
	node << temp;
	return Create(node);
}

std::shared_ptr<Mesh> Mesh::Create(const std::string& importPath)
{
	Mesh temp(importPath);
	Node node;
	node << temp;
	return Create(node);
}

std::shared_ptr<Mesh> Mesh::Create(const Node& node)
{
	if (auto resource = Resources::Get()->Find<Mesh>(node)) {
		std::cout << "Reusing old mesh at: " << resource->filename << std::endl;
		return resource;
	}

	auto result = std::make_shared<Mesh>("");
	Resources::Get()->Add(node, std::dynamic_pointer_cast<Resource>(result));
	node >> *result;
	result->Load();
	return result;
}

void Mesh::Load()
{
	std::cout << "Instantiated mesh from path: " << filename << std::endl;

	//std::vector<Vertex> vertices;

	/*
	constexpr float R2 = 1.0f; //tube radius

	constexpr uint32_t U_STEPS = 20;
	constexpr uint32_t V_STEPS = 16;

	//texture repeats around the torus:
	constexpr float V_REPEATS = 2.0f;
	float U_REPEATS = std::ceil(V_REPEATS / R2);

	auto emplace_vertex = [&](uint32_t ui, uint32_t vi) {
		//convert steps to angles:
		// (doing the mod since trig on 2 M_PI may not exactly match 0)
		float ua = (ui % U_STEPS) / float(U_STEPS) * 2.0f * Math::PI<float>;
		float va = (vi % V_STEPS) / float(V_STEPS) * 2.0f * Math::PI<float>;

		float x = (R2 * std::cos(va)) * std::cos(ua);
		float y = (R2 * std::cos(va)) * std::sin(ua);
		float z = R2 * std::sin(va);
		float L = std::sqrt(x * x + y * y + z * z);

		vertices.emplace_back(PosNorTexVertex{
			.Position{x, y, z},
			.Normal{
				.x = x / L,
				.y = y / L,
				.z = z / L,
			},
			.TexCoord{
				.s = ui / float(U_STEPS) * U_REPEATS,
				.t = vi / float(V_STEPS) * V_REPEATS,
			},
			});
		};

	for (uint32_t ui = 0; ui < U_STEPS; ++ui) {
		for (uint32_t vi = 0; vi < V_STEPS; ++vi) {
			emplace_vertex(ui, vi);
			emplace_vertex(ui + 1, vi);
			emplace_vertex(ui, vi + 1);

			emplace_vertex(ui, vi + 1);
			emplace_vertex(ui + 1, vi);
			emplace_vertex(ui + 1, vi + 1);
		}
	}

	size_t bytes = vertices.size() * sizeof(vertices[0]);
	*/

	//assert(vertices.size() == 20 * 16 * 6);

	const std::string fullPath = "../scenes/examples/" + filename;
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
	node["filename"].Get(mesh.filename);
	node["numVertices"].Get(mesh.numVertices);
	return node;
}

Node& operator<<(Node& node, const Mesh& mesh) {
	node["filename"].Set(mesh.filename);
	node["numVertices"].Set(mesh.numVertices);
	return node;
}