#include "Mesh.hpp"

#include "math/Math.hpp"
#include <iostream>

Mesh::Mesh(std::filesystem::path importPath) :
	filename(importPath.string()) {
}

Mesh::~Mesh()
{
	m_VertexBuffer.Destroy();
}

std::shared_ptr<Mesh> Mesh::Create(std::filesystem::path& importPath)
{
	Mesh temp(importPath);
	Node node;
	node << temp;
	return Create(node);
}

std::shared_ptr<Mesh> Mesh::Create(const Node& node)
{
	if (auto resource = Resources::Get()->Find<Mesh>(node)) {
		std::cout << "Found old mesh...\n";
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
	std::cout << "Instantiated mesh!>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n";

	std::vector<Vertex> vertices;

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

	m_VertexBuffer = Buffer(
		bytes,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	assert(vertices.size() == 20 * 16 * 6);

	//copy data to buffer:
	Buffer::TransferToBuffer(vertices.data(), bytes, m_VertexBuffer.getBuffer());

	numVertices = static_cast<uint32_t>(vertices.size());
}

const Node& operator>>(const Node& node, Mesh& mesh) {
	node["filename"].Get(mesh.filename);
	return node;
}

Node& operator<<(Node& node, const Mesh& mesh) {
	node["filename"].Set(mesh.filename);
	return node;
}