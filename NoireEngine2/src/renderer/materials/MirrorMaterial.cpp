#include "MirrorMaterial.hpp"
#include "imgui/imgui.h"

MirrorMaterial::MirrorMaterial(const std::string& name_) :
	name(name_)
{
}

std::shared_ptr<Material> MirrorMaterial::Create(const std::string& name_)
{
	MirrorMaterial temp(name_);
	Node node;
	node << temp;
	return Create(node);
}

Material* MirrorMaterial::Deserialize(const Scene::TValueMap& obj)
{
	return MirrorMaterial::Create(obj.at("name").as_string().value()).get();
}

void MirrorMaterial::Push(const CommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout)
{
	// TODO: add some pushes here for texture dynamic indexing
}

void MirrorMaterial::Inspect()
{
	ImGui::PushID("###MaterialName");
	ImGui::Columns(2);
	ImGui::Text("Material Name");
	ImGui::NextColumn();
	ImGui::Text(name.c_str());
	ImGui::Columns(1);
	ImGui::PopID();

	ImGui::PushID("###MaterialWorkflow");
	ImGui::Columns(2);
	ImGui::Text("Workflow");
	ImGui::NextColumn();
	ImGui::Text("Mirror");
	ImGui::Columns(1);
	ImGui::PopID();
}

std::shared_ptr<Material> MirrorMaterial::Create(const Node& node)
{
	if (auto resource = Resources::Get()->Find<Material>(node)) {
		return resource;
	}

	auto result = std::make_shared<MirrorMaterial>();
	Resources::Get()->Add(node, std::dynamic_pointer_cast<Resource>(result));
	node >> *result;
	return result;
}

const Node& operator>>(const Node& node, MirrorMaterial& material)
{
	node["name"].Get(material.name);
	node["workflow"].Get(material.getWorkflow());
	return node;
}

Node& operator<<(Node& node, const MirrorMaterial& material)
{
	node["name"].Set(material.name);
	node["workflow"].Set(material.getWorkflow());
	return node;
}