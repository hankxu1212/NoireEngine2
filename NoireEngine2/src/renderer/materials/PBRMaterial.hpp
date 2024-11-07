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

	struct MaterialUniform
	{
		glm::vec4 albedo = glm::vec4(1);
		float environmentLightIntensity = 1;
		int albedoTexId = -1;
		int normalTexId = -1;
		int displacementTexId = -1;
		float heightScale = 0.1f;
		int roughnessTexId = -1;
		int metallicTexId = -1;
		float roughness = 0.5f;
		float metallic = 0;
		float normalStrength = 1;
	};

public:
	PBRMaterial() = default;

	static std::shared_ptr<Material> Create();

	static std::shared_ptr<Material> Create(const CreateInfo& createInfo);

	void Load();

	static Material* Deserialize(const Scene::TValueMap& obj);

	friend const Node& operator>>(const Node& node, PBRMaterial& material);
	friend Node& operator<<(Node& node, const PBRMaterial& material);

	friend const Node& operator>>(const Node& node, CreateInfo& info);
	friend Node& operator<<(Node& node, const CreateInfo& info);

	void Inspect() override;

	Workflow getWorkflow() const override { return Workflow::PBR; }

	void* getPushPointer() const override { return (void*)&m_Uniform; };

private:
	PBRMaterial(const CreateInfo& createInfo);
	static std::shared_ptr<Material> Create(const Node& node);

	CreateInfo						m_CreateInfo;
	MaterialUniform					m_Uniform;
};
