#pragma once

#include "Material.hpp"
#include "renderer/scene/Scene.hpp"

class PBRMaterial : public Material
{
public:
	struct CreateInfo
	{
		std::string name = "Lit PBR";
		std::string texturePath = NE_NULL_STR;
		std::string normalPath = NE_NULL_STR;
		std::string displacementPath = NE_NULL_STR;
		std::string roughnessPath = NE_NULL_STR;
		std::string metallicPath = NE_NULL_STR;
		glm::vec3 albedo = glm::vec3(1);
		float roughness = 0.5f;
		float metallic = 0;

		CreateInfo() = default;
	};

	struct MaterialPush
	{
		glm::vec4 albedo;
		float environmentLightIntensity;
		int albedoTexId;
		int normalTexId;
		int displacementTexId;
		float heightScale;
		int roughnessTexId;
		int metallicTexId;
		float roughness;
		float metallic;
		float normalStrength;
	}push;

public:
	PBRMaterial() = default;

	static std::shared_ptr<Material> Create();

	static std::shared_ptr<Material> Create(const CreateInfo& createInfo);

	void Load();

	void Push(const CommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout) override;

	static Material* Deserialize(const Scene::TValueMap& obj);

	friend const Node& operator>>(const Node& node, PBRMaterial& material);
	friend Node& operator<<(Node& node, const PBRMaterial& material);

	friend const Node& operator>>(const Node& node, CreateInfo& info);
	friend Node& operator<<(Node& node, const CreateInfo& info);

	void Inspect() override;

	Workflow getWorkflow() const override { return Workflow::PBR; }

	void* getPushPointer() const override { return (void*)&push; };

private:
	PBRMaterial(const CreateInfo& createInfo);
	static std::shared_ptr<Material> Create(const Node& node);

	CreateInfo						m_CreateInfo;
	int								m_AlbedoMapId = -1;
	int								m_NormalMapId = -1;
	int								m_DisplacementMapId = -1;
	float							m_HeightScale = 0.1f;
	int								m_RoughnessMapId = -1;
	int								m_MetallicMapId = -1;
	float							m_NormalStrength = 1;
	float							m_EnvironmentLightInfluence = 1;
};
