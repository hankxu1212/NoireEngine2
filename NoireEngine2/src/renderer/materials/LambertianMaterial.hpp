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

	struct MaterialPush
	{
		glm::vec4 albedo;
		float environmentLightIntensity;
		float normalStrength;
		int albedoTexId;
		int normalTexId;
	} push;

public:
	LambertianMaterial() = default;

	static std::shared_ptr<Material> Create();

	static std::shared_ptr<Material> Create(const CreateInfo& createInfo);

	void Load();

	void Push(const CommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout) override;

	static Material* Deserialize(const Scene::TValueMap& obj);

	friend const Node& operator>>(const Node& node, LambertianMaterial& material);
	friend Node& operator<<(Node& node, const LambertianMaterial& material);

	friend const Node& operator>>(const Node& node, CreateInfo& info);
	friend Node& operator<<(Node& node, const CreateInfo& info);

	void Inspect() override;

	Workflow getWorkflow() const override { return Workflow::Lambertian; }

	void* getPushPointer() const override { return (void*)&push; };

private:
	LambertianMaterial(const CreateInfo& createInfo);
	static std::shared_ptr<Material> Create(const Node& node);

	CreateInfo						m_CreateInfo;
	int								m_AlbedoMapId = -1; // index into global texture array
	int								m_NormalMapId = -1; // index into global texture array
	float							m_NormalStrength = 1;
	float							m_EnvironmentLightInfluence = 0.1f;
};
