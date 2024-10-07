#include "EnvironmentMaterial.hpp"
#include "imgui/imgui.h"

EnvironmentMaterial::EnvironmentMaterial(const std::string& name_) :
	name(name_)
{
}

std::shared_ptr<Material> EnvironmentMaterial::Create(const std::string& name_)
{
    EnvironmentMaterial temp(name_);
    Node node;
    node << temp;
    return Create(node);
}

Material* EnvironmentMaterial::Deserialize(const Scene::TValueMap& obj)
{
	return EnvironmentMaterial::Create(obj.at("name").as_string().value()).get();
}

void EnvironmentMaterial::Push(const CommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout)
{
	// TODO: add some pushes here for texture dynamic indexing
}

void EnvironmentMaterial::Inspect()
{
	ImGui::PushID("###MaterialName");
	ImGui::Columns(2);
	ImGui::Text("%s", "Material Name");
	ImGui::NextColumn();
	ImGui::Text(name.c_str());
	ImGui::Columns(1);
	ImGui::PopID();

	ImGui::PushID("###MaterialWorkflow");
	ImGui::Columns(2);
	ImGui::Text("Workflow");
	ImGui::NextColumn();
	ImGui::Text("Environment");
	ImGui::Columns(1);
	ImGui::PopID();
}

std::shared_ptr<Material> EnvironmentMaterial::Create(const Node& node)
{
    if (auto resource = Resources::Get()->Find<Material>(node)) {
        return resource;
    }

    auto result = std::make_shared<EnvironmentMaterial>();
    Resources::Get()->Add(node, std::dynamic_pointer_cast<Resource>(result));
    node >> *result;
    //result->Load();
    return result;
}

const Node& operator>>(const Node& node, EnvironmentMaterial& material)
{
	node["name"].Get(material.name);
	node["workflow"].Get(material.getWorkflow());
	return node;
}

Node& operator<<(Node& node, const EnvironmentMaterial& material)
{
	node["name"].Set(material.name);
	node["workflow"].Set(material.getWorkflow());
	return node;
}