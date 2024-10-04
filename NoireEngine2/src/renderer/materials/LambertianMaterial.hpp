#pragma once

#include "Material.hpp"
#include "renderer/scene/Scene.hpp"

class LambertianMaterial : public Material
{
public:
	struct CreateInfo
	{
		std::string name = "Default Lambertian";
		glm::vec3 albedo = glm::vec3(1);
		std::string texturePath;

		CreateInfo() = default;

		CreateInfo(const std::string& n, const glm::vec3& a, const std::string& tPath) :
			name(n), albedo(a), texturePath(tPath) {
		}

		CreateInfo(const CreateInfo& other) :
			name(other.name), albedo(other.albedo), texturePath(other.texturePath) {
		}
	};

	struct MaterialPush
	{
		struct { float x, y, z, padding_; } albedo;
		int texIndex;
	};
	static_assert(sizeof(MaterialPush) == 16 + 4);

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

private:
	LambertianMaterial(const CreateInfo& createInfo);
	static std::shared_ptr<Material> Create(const Node& node);

	CreateInfo						m_CreateInfo;
	uint32_t						m_AlbedoMapIndex = 0; // index into global texture array
};
