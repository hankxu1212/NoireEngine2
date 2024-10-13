#include "PBRMaterial.hpp"
#include "utils/Logger.hpp"
#include "backend/images/Image2D.hpp"
#include "core/resources/Files.hpp"
#include "renderer/scene/SceneManager.hpp"

#include "editor/ImGuiExtension.hpp"
#include <limits>

void PBRMaterial::Push(const CommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout)
{
	PBRMaterial::MaterialPush push
	{
		.albedo = { m_CreateInfo.albedo.x, m_CreateInfo.albedo.y, m_CreateInfo.albedo.z, 0 },
		.albedoTexId = m_AlbedoMapId,
		.normalTexId = m_NormalMapId,
		.displacementTexId = m_DisplacementMapId,
		.heightScale = m_HeightScale,
		.roughnessTexId = m_RoughnessMapId,
		.metallicTexId = m_MetallicMapId,
		.roughness = m_CreateInfo.roughness,
		.metallic = m_CreateInfo.metallic,
		.normalStrength = m_NormalStrength,
		.environmentLightIntensity = m_EnvironmentLightInfluence,
	};

	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
		sizeof(PBRMaterial::MaterialPush), &push);
}

Material* PBRMaterial::Deserialize(const Scene::TValueMap& obj)
{
	try {
		CreateInfo createInfo;
		createInfo.name = obj.at("name").as_string().value();

		const auto& attributesMap = obj.at("pbr").as_object().value();

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

		// metallic
		{
			const auto& metallicVal = attributesMap.at("metalness");
			if (const auto& metallicTex = metallicVal.as_texPath())
			{
				createInfo.metallicPath = metallicTex.value();
			}
			else
				createInfo.metallic = metallicVal.as_float();
		}

		// roughness
		{
			const auto& roughnessVal = attributesMap.at("roughness");
			if (const auto& roughTex = roughnessVal.as_texPath())
			{
				createInfo.roughnessPath = roughTex.value();
			}
			else
				createInfo.roughness = roughnessVal.as_float();
		}

		return Create(createInfo).get();
	}
	catch (std::exception& e) {
		NE_WARN("Failed to deserialize value as Material: {}", e.what());
		return nullptr;
	}
}

PBRMaterial::PBRMaterial(const CreateInfo& createInfo) :
	m_CreateInfo(createInfo) {
}

std::shared_ptr<Material> PBRMaterial::Create()
{
	CreateInfo defaultInfo;
	return Create(defaultInfo);
}

std::shared_ptr<Material> PBRMaterial::Create(const CreateInfo& createInfo)
{
	PBRMaterial temp(createInfo);
	Node node;
	node << temp;
	return Create(node);
}

std::shared_ptr<Material> PBRMaterial::Create(const Node& node)
{
	if (auto resource = Resources::Get()->Find<Material>(node)) {
		return resource;
	}

	auto result = std::make_shared<PBRMaterial>();
	Resources::Get()->Add(node, std::dynamic_pointer_cast<Resource>(result));
	node >> *result;
	result->Load();
	return result;
}

void PBRMaterial::Load()
{
	const auto& rootPath = SceneManager::Get()->getScene()->getRootPath();

	// albedo
	if (m_CreateInfo.texturePath != NE_NULL_STR)
	{
		auto tex = Image2D::Create(rootPath.parent_path() / m_CreateInfo.texturePath);
		m_AlbedoMapId = tex->getTextureId();
	}

	// normal
	if (m_CreateInfo.normalPath != NE_NULL_STR)
	{
		auto tex = Image2D::Create(rootPath.parent_path() / m_CreateInfo.normalPath);
		m_NormalMapId = tex->getTextureId();
	}

	// TODO: combine normal and displacement map
	if (m_CreateInfo.displacementPath != NE_NULL_STR)
	{
		auto tex = Image2D::Create(rootPath.parent_path() / m_CreateInfo.displacementPath);
		m_DisplacementMapId = tex->getTextureId();
	}

	// metallic
	if (m_CreateInfo.metallicPath != NE_NULL_STR)
	{
		auto tex = Image2D::Create(rootPath.parent_path() / m_CreateInfo.metallicPath);
		m_MetallicMapId = tex->getTextureId();
	}

	// metallic
	if (m_CreateInfo.roughnessPath != NE_NULL_STR)
	{
		auto tex = Image2D::Create(rootPath.parent_path() / m_CreateInfo.roughnessPath);
		m_RoughnessMapId = tex->getTextureId();
	}
}

void PBRMaterial::Inspect()
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
	ImGui::Text("PBR");
	ImGui::Columns(1);
	ImGui::PopID();

	ImGui::PushID("###Albedo");
	ImGui::ColorEdit3("Albedo", (float*)&m_CreateInfo.albedo);
	ImGui::PopID();

	static const char* albedoIDs[]{ "###ALBEDOPATH",  "###ALBEDOID" };
	ImGuiExt::InspectTexture(albedoIDs, "Albedo Texture", m_CreateInfo.texturePath.c_str(), &m_AlbedoMapId);

	static const char* normalIDs[]{ "###NORMALPATH",  "###NORMALID", "###NORMALSTRENGTH"};
	ImGuiExt::InspectTexture(normalIDs, "Normal Texture", m_CreateInfo.normalPath.c_str(), &m_NormalMapId, &m_NormalStrength);

	static const char* displaceIDs[]{ "###DISPPATH",  "###DISPID", "###DISPHEIGHT" };
	ImGuiExt::InspectTexture(displaceIDs, "Displacement Texture", m_CreateInfo.displacementPath.c_str(), &m_DisplacementMapId, &m_HeightScale);

	static const char* roughIDs[]{ "###ROUGHPATH",  "###ROUGHID", "###ROUGHNESS" };
	ImGuiExt::InspectTexture(roughIDs, "Roughness Texture", m_CreateInfo.roughnessPath.c_str(), &m_RoughnessMapId, &m_CreateInfo.roughness, 1);

	static const char* metallicIDs[]{ "###METALLICPATH",  "###METALLICID", "###METALLIC" };
	ImGuiExt::InspectTexture(metallicIDs, "Metallic Texture", m_CreateInfo.metallicPath.c_str(), &m_MetallicMapId, &m_CreateInfo.metallic, 1);

	ImGui::SeparatorText("Environmental Lighting");
	ImGui::PushID("###EnvironmentLightingIntensity");
	ImGui::Columns(2);
	ImGui::Text("Environment Lighting Influence");
	ImGui::NextColumn();
	ImGui::DragFloat("####ETI", &m_EnvironmentLightInfluence, 0.01f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
	ImGui::Columns(1);
	ImGui::PopID();
}

const Node& operator>>(const Node& node, PBRMaterial& material)
{
	node["createInfo"].Get(material.m_CreateInfo);
	node["workflow"].Get(material.getWorkflow());
	node["albedoTexId"].Get(material.m_AlbedoMapId);
	node["normalTexId"].Get(material.m_NormalMapId);
	node["normalStrength"].Get(material.m_NormalStrength);
	node["displacementTexId"].Get(material.m_DisplacementMapId);
	node["roughnessTexId"].Get(material.m_RoughnessMapId);
	node["metallicTexId"].Get(material.m_MetallicMapId);
	node["environmentInfluence"].Get(material.m_EnvironmentLightInfluence);
	return node;
}

Node& operator<<(Node& node, const PBRMaterial& material)
{
	node["createInfo"].Set(material.m_CreateInfo);
	node["workflow"].Set(material.getWorkflow());
	node["albedoTexId"].Set(material.m_AlbedoMapId);
	node["normalTexId"].Set(material.m_NormalMapId);
	node["normalStrength"].Set(material.m_NormalStrength);
	node["displacementTexId"].Set(material.m_DisplacementMapId);
	node["roughnessTexId"].Set(material.m_RoughnessMapId);
	node["metallicTexId"].Set(material.m_MetallicMapId);
	node["environmentInfluence"].Set(material.m_EnvironmentLightInfluence);
	return node;
}

const Node& operator>>(const Node& node, PBRMaterial::CreateInfo& info)
{
	node["albedo"].Get(info.albedo);
	node["name"].Get(info.name);
	node["albedoTex"].Get(info.texturePath);
	node["normalTex"].Get(info.normalPath);
	node["displacementTex"].Get(info.displacementPath);
	node["roughnessTex"].Get(info.roughnessPath);
	node["metallicTex"].Get(info.metallicPath);
	node["metallic"].Get(info.metallic);
	node["roughness"].Get(info.roughness);
	return node;
}

Node& operator<<(Node& node, const PBRMaterial::CreateInfo& info)
{
	node["albedo"].Set(info.albedo);
	node["name"].Set(info.name);
	node["albedoTex"].Set(info.texturePath);
	node["normalTex"].Set(info.normalPath);
	node["displacementTex"].Set(info.displacementPath);
	node["roughnessTex"].Set(info.roughnessPath);
	node["metallicTex"].Set(info.metallicPath);
	node["metallic"].Set(info.metallic);
	node["roughness"].Set(info.roughness);
	return node;
}