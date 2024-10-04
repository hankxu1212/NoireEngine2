#pragma once

#include "core/resources/Resources.hpp"

#include <vulkan/vulkan.h>
#include "backend/commands/CommandBuffer.hpp"
#include "renderer/scene/Scene.hpp"

class MaterialPipeline;

class Material : public Resource
{
public:
	enum class Workflow
	{
		Lambertian = 0,
		PBR = 1,
		Environment = 2,
		Mirror = 3
	};

public:
	struct CreateInfo
	{
		std::string name;
		glm::vec3 albedo;

		CreateInfo() = default;
		
		CreateInfo(const std::string& name_, glm::vec3& albedo_)
		{
			name.assign(name_);
			albedo = albedo_;
		}

		CreateInfo(const CreateInfo& other)
		{
			name.assign(other.name);
			albedo = other.albedo;
		}
	};

public:
	Material() = default;
	Material(const CreateInfo& createInfo);

	virtual std::type_index getTypeIndex() const { return typeid(Material); }

	virtual void Push(const CommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout);

	static Material* Deserialize(const Scene::TValueMap& obj);

	static std::shared_ptr<Material> CreateDefault();
	static std::shared_ptr<Material> Create(const CreateInfo& createInfo);
	static std::shared_ptr<Material> Create(const Node& node);

	friend const Node& operator>>(const Node& node, Material& material);
	friend Node& operator<<(Node& node, const Material& material);

	friend const Node& operator>>(const Node& node, CreateInfo& info);
	friend Node& operator<<(Node& node, const CreateInfo& info);

private:
	CreateInfo						m_CreateInfo;
	glm::vec3						m_Albedo;
	MaterialPipeline*				p_MaterialPipeline;
};

