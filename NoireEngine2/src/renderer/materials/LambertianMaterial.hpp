#pragma once

#include "Material.hpp"
#include "renderer/scene/Scene.hpp"

class LambertianMaterial : public Material
{
public:
	struct CreateInfo
	{
		std::string name = "Lit Lambertian";
		glm::vec3 albedo = glm::vec3(1);
		std::string texturePath = NE_NULL_STR;
		std::string normalPath = NE_NULL_STR;
		std::string displacementPath = NE_NULL_STR;

		CreateInfo() = default;
	};

	struct MaterialUniform
	{
		glm::vec4 albedo;
		float environmentLightIntensity = 1;
		float normalStrength = 1;
		int albedoTexId = -1;
		int normalTexId = -1;
	};

public:
	LambertianMaterial() = default;

	static std::shared_ptr<Material> Create();

	static std::shared_ptr<Material> Create(const CreateInfo& createInfo);

	void Load();

	static Material* Deserialize(const Scene::TValueMap& obj);

	friend const Node& operator>>(const Node& node, LambertianMaterial& material);
	friend Node& operator<<(Node& node, const LambertianMaterial& material);

	friend const Node& operator>>(const Node& node, CreateInfo& info);
	friend Node& operator<<(Node& node, const CreateInfo& info);

	void Inspect() override;

	Workflow getWorkflow() const override { return Workflow::Lambertian; }

	void* getPushPointer() const override { return (void*)&m_Uniform; };

private:
	LambertianMaterial(const CreateInfo& createInfo);
	static std::shared_ptr<Material> Create(const Node& node);

	MaterialUniform m_Uniform;
	CreateInfo m_CreateInfo;
};
