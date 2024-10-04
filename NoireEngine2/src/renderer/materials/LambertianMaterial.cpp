#include "LambertianMaterial.hpp"
#include "utils/Logger.hpp"
#include "backend/images/Image2D.hpp"
#include "core/resources/Files.hpp"

#include "imgui/imgui.h"
#include <limits>

void LambertianMaterial::Push(const CommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout)
{
	LambertianMaterial::MaterialPush push{
		.albedo = { m_CreateInfo.albedo.x, m_CreateInfo.albedo.y, m_CreateInfo.albedo.z, 0 },
		.texIndex = (int)m_AlbedoMapIndex
	};

	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
		sizeof(LambertianMaterial::MaterialPush), &push);
}

Material* LambertianMaterial::Deserialize(const Scene::TValueMap& obj)
{
	try {
		const auto& attributesMap = obj.at("lambertian").as_object().value();
		CreateInfo createInfo;
		createInfo.name = obj.at("name").as_string().value();

		auto attributeIt = attributesMap.find("albedo");
		if (attributeIt == attributesMap.end())
			throw std::runtime_error("Did not find albedo field in this material, aborting!");

		if (auto& albedoTexOpt = attributeIt->second.as_texPath())
			createInfo.texturePath = albedoTexOpt.value();
		else
			createInfo.albedo = attributeIt->second.as_vec3();

		return Create(createInfo).get();
	}
	catch (std::exception& e) {
		NE_WARN("Failed to deserialize value as Material: {}", e.what());
		return nullptr;
	}
}

LambertianMaterial::LambertianMaterial(const CreateInfo& createInfo) :
	m_CreateInfo(createInfo) {
}

std::shared_ptr<Material> LambertianMaterial::Create()
{
	CreateInfo defaultInfo;
	return Create(defaultInfo);
}

std::shared_ptr<Material> LambertianMaterial::Create(const CreateInfo& createInfo)
{
	LambertianMaterial temp(createInfo);
	Node node;
	node << temp;
	return Create(node);
}

std::shared_ptr<Material> LambertianMaterial::Create(const Node& node)
{
	if (auto resource = Resources::Get()->Find<Material>(node)) {
		return resource;
	}

	auto result = std::make_shared<LambertianMaterial>();
	Resources::Get()->Add(node, std::dynamic_pointer_cast<Resource>(result));
	node >> *result;
	result->Load();
	return result;
}

void LambertianMaterial::Load()
{
	if (m_CreateInfo.texturePath != "")
	{
		Image2D::Create(Files::Path("../scenes/examples/" + m_CreateInfo.texturePath));
	}
}

void LambertianMaterial::Inspect()
{
	ImGui::PushID("###MaterialName");
	ImGui::Columns(2);
	ImGui::Text("%s", "Material Name");
	ImGui::NextColumn();
	ImGui::Text(m_CreateInfo.name.c_str());
	ImGui::Columns(1);
	ImGui::PopID();

	ImGui::PushID("###MaterialWorkflow");
	ImGui::Columns(2);
	ImGui::Text("Workflow");
	ImGui::NextColumn();
	ImGui::Text("Lambertian");
	ImGui::Columns(1);
	ImGui::PopID();

	ImGui::PushID("###Albedo");
	ImGui::ColorEdit3("Albedo", (float*)&m_CreateInfo.albedo);
	ImGui::PopID();

	ImGui::PushID("###AlbedoTexture");
	ImGui::Columns(2);
	ImGui::Text("%s", "Albedo Texture");
	ImGui::NextColumn();
	ImGui::Text(m_CreateInfo.texturePath.c_str());
	ImGui::Columns(1);
	ImGui::PopID();

	ImGui::DragInt("Albedo Texture Index", (int*)&m_AlbedoMapIndex, 1, 0, 4096);
}

const Node& operator>>(const Node& node, LambertianMaterial& material)
{
	node["createInfo"].Get(material.m_CreateInfo);
	node["albedoTexIndex"].Get(material.m_AlbedoMapIndex);
	node["workflow"].Get(material.getWorkflow());
	return node;
}

Node& operator<<(Node& node, const LambertianMaterial& material)
{
	node["createInfo"].Set(material.m_CreateInfo);
	node["albedoTexIndex"].Set(material.m_AlbedoMapIndex);
	node["workflow"].Set(material.getWorkflow());
	return node;
}

const Node& operator>>(const Node& node, LambertianMaterial::CreateInfo& info)
{
	node["albedo"].Get(info.albedo);
	node["name"].Get(info.name);
	node["albedoTex"].Get(info.texturePath);
	return node;
}

Node& operator<<(Node& node, const LambertianMaterial::CreateInfo& info)
{
	node["albedo"].Set(info.albedo);
	node["name"].Set(info.name);
	node["albedoTex"].Set(info.texturePath);
	return node;
}

const Node& operator>>(const Node& node, glm::vec3& v) {
	node["x"].Get(v.x);
	node["y"].Get(v.y);
	node["z"].Get(v.z);
	return node;
}

Node& operator<<(Node& node, const glm::vec3& v) {
	node["x"].Set(v.x);
	node["y"].Set(v.y);
	node["z"].Set(v.z);
	return node;
}