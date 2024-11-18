#pragma once

#include "Material.hpp"
#include "renderer/scene/Scene.hpp"

class GlassMaterial : public Material
{
public:
	struct CreateInfo
	{
		std::string name = "Lit PBR";
		float IOR = 0.5f;

		CreateInfo() = default;
	};

	struct MaterialUniform
	{
		float IOR = 0.5f;
	};

public:
	GlassMaterial() = default;

	void Load() override;

	static Material* Deserialize(const Scene::TValueMap& obj);

	friend const Node& operator>>(const Node& node, GlassMaterial& material);
	friend Node& operator<<(Node& node, const GlassMaterial& material);

	friend const Node& operator>>(const Node& node, CreateInfo& info);
	friend Node& operator<<(Node& node, const CreateInfo& info);

	void Inspect() override;

	Workflow getWorkflow() const override { return Workflow::Glass; }

	void* getPushPointer() const override { return (void*)&m_Uniform; };

private:
	friend class Material;

	GlassMaterial(const CreateInfo& createInfo);

	CreateInfo						m_CreateInfo;
	MaterialUniform					m_Uniform;
};
