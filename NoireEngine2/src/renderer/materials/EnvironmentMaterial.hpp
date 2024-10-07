#pragma once

#include "Material.hpp"
#include "renderer/scene/Scene.hpp"

class EnvironmentMaterial : public Material
{
public:
	EnvironmentMaterial() = default;

	static std::shared_ptr<Material> Create(const std::string& createInfo);

	static Material* Deserialize(const Scene::TValueMap& obj);

	void Push(const CommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout) override;

	friend const Node& operator>>(const Node& node, EnvironmentMaterial& material);
	friend Node& operator<<(Node& node, const EnvironmentMaterial& material);

	void Inspect() override;

	Workflow getWorkflow() const override { return Workflow::Environment; }

private:
	EnvironmentMaterial(const std::string&);

	static std::shared_ptr<Material> Create(const Node& node);

	std::string name;
};

