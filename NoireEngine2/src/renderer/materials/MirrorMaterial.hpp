#pragma once

#include "Material.hpp"
#include "renderer/scene/Scene.hpp"

class MirrorMaterial : public Material
{
public:
	MirrorMaterial() = default;

	static std::shared_ptr<Material> Create(const std::string& createInfo);

	static Material* Deserialize(const Scene::TValueMap& obj);

	void Push(const CommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout) override;

	friend const Node& operator>>(const Node& node, MirrorMaterial& material);
	friend Node& operator<<(Node& node, const MirrorMaterial& material);

	void Inspect() override;

	Workflow getWorkflow() const override { return Workflow::Mirror; }

private:
	MirrorMaterial(const std::string&);

	static std::shared_ptr<Material> Create(const Node& node);

	std::string name;
};

