#include "LambertianMaterial.hpp"
#include "utils/Logger.hpp"
#include "backend/images/Image2D.hpp"
#include "core/resources/Files.hpp"
#include "renderer/scene/SceneManager.hpp"
#include "core/Timer.hpp"

#include "editor/ImGuiExtension.hpp"
#include <limits>

Material* LambertianMaterial::Deserialize(const Scene::TValueMap& obj)
{
	try {
		const auto& attributesMap = obj.at("lambertian").as_object().value();
		CreateInfo createInfo;
		createInfo.name = obj.at("name").as_string().value();

		// find albedo map
		{
			auto attributeIt = attributesMap.find("albedo");
			if (attributeIt == attributesMap.end())
				throw std::runtime_error("Did not find albedo field in this material, aborting!");

			if (auto& albedoTexOpt = attributeIt->second.as_texPath())
				createInfo.texturePath = albedoTexOpt.value();
			else
				createInfo.albedo = attributeIt->second.as_vec3();
		}

		// find normal map
		{
			auto materialNormalIt = obj.find("normalMap");
			if (materialNormalIt != obj.end()) 
			{
				if (auto& normalTexOpt = materialNormalIt->second.as_texPath()) {
					createInfo.normalPath = normalTexOpt.value();
					NE_INFO("Found normal map on this entity:{}", createInfo.normalPath);
				}
			}
		}

		// find displacement map
		{
			auto materialHeightIt = obj.find("displacementMap");
			if (materialHeightIt != obj.end())
			{
				if (auto& heightTexOpt = materialHeightIt->second.as_texPath()) {
					createInfo.displacementPath = heightTexOpt.value();
					NE_INFO("Found displacement map on this entity:{}", createInfo.displacementPath);
				}
			}
		}

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
	const auto& rootPath = SceneManager::Get()->getScene()->getRootPath();
	if (m_CreateInfo.texturePath != NE_NULL_STR)
	{
		auto tex = Image2D::Create(rootPath.parent_path() / m_CreateInfo.texturePath);
		m_Uniform.albedoTexId = tex->getID();
	}
	if (m_CreateInfo.normalPath != NE_NULL_STR)
	{
		auto tex = Image2D::Create(rootPath.parent_path() / m_CreateInfo.normalPath, VK_FORMAT_R8G8B8A8_UNORM, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, true, false, true);
		m_Uniform.normalTexId = tex->getID();
	}

	m_Uniform.albedo = glm::vec4(m_CreateInfo.albedo, 0);
}

void LambertianMaterial::Inspect()
{
	ImGui::PushID("###MaterialName");
	ImGui::Columns(2);
	ImGui::Text("Material Name");
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
	ImGui::ColorEdit3("Albedo", (float*)&m_Uniform.albedo);
	ImGui::PopID();

	static const char* albedoIDs[]{ "###ALBEDOPATH",  "###ALBEDOID" };
	ImGuiExt::InspectTexture(albedoIDs, "Albedo Texture", m_CreateInfo.texturePath.c_str(), &m_Uniform.albedoTexId);

	static const char* normalIDs[]{ "###NORMALPATH",  "###NORMALID", "###NORMALSTRENGTH" };
	ImGuiExt::InspectTexture(normalIDs, "Normal Texture", m_CreateInfo.normalPath.c_str(), &m_Uniform.normalTexId, &m_Uniform.normalStrength);

	ImGui::PushID("###EnvironmentLightingIntensity");
	ImGui::Columns(2);
	ImGui::Text("Environment Lighting Influence");
	ImGui::NextColumn();
	ImGui::DragFloat("####ETI", &m_Uniform.environmentLightIntensity, 0.01f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
	ImGui::Columns(1);
	ImGui::PopID();
}

const Node& operator>>(const Node& node, LambertianMaterial& material)
{
	node["createInfo"].Get(material.m_CreateInfo);
	node["albedoTexId"].Get(material.m_Uniform.albedoTexId);
	node["normalTexId"].Get(material.m_Uniform.normalTexId);
	node["normalStrength"].Get(material.m_Uniform.normalStrength);
	node["environmentInfluence"].Get(material.m_Uniform.environmentLightIntensity);
	node["workflow"].Get(material.getWorkflow());
	return node;
}

Node& operator<<(Node& node, const LambertianMaterial& material)
{
	node["createInfo"].Set(material.m_CreateInfo);
	node["albedoTexId"].Set(material.m_Uniform.albedoTexId);
	node["normalTexId"].Set(material.m_Uniform.normalTexId);
	node["normalStrength"].Set(material.m_Uniform.normalStrength);
	node["environmentInfluence"].Set(material.m_Uniform.environmentLightIntensity);
	node["workflow"].Set(material.getWorkflow());
	return node;
}

const Node& operator>>(const Node& node, LambertianMaterial::CreateInfo& info)
{
	node["albedo"].Get(info.albedo);
	node["name"].Get(info.name);
	node["albedoTex"].Get(info.texturePath);
	node["normalTex"].Get(info.normalPath);
	node["displacementTex"].Get(info.displacementPath);
	return node;
}

Node& operator<<(Node& node, const LambertianMaterial::CreateInfo& info)
{
	node["albedo"].Set(info.albedo);
	node["name"].Set(info.name);
	node["albedoTex"].Set(info.texturePath);
	node["normalTex"].Set(info.normalPath);
	node["displacementTex"].Set(info.displacementPath);
	return node;
}