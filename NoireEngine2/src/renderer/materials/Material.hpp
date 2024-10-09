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
		Environment = 1,
		Mirror = 2,
		PBR = 3,
	};

public:
	virtual std::type_index getTypeIndex() const { return typeid(Material); }

	virtual void Push(const CommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout) {}

	virtual void Inspect() {}
	virtual void Debug() {}

	static Material* Deserialize(const Scene::TValueMap& obj);

	static std::shared_ptr<Material> CreateDefault();

	virtual Workflow getWorkflow() const = 0;
};

