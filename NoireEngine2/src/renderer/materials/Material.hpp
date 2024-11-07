#pragma once

#include "core/resources/Resources.hpp"

#include <vulkan/vulkan.h>
#include "backend/commands/CommandBuffer.hpp"
#include "renderer/scene/Scene.hpp"

class MaterialPipeline;

#define NUM_WORKFLOWS 2

class Material : public Resource
{
public:
	enum class Workflow
	{
		Lambertian = 0,
		PBR = 1,
	};

public:
	virtual std::type_index getTypeIndex() const { return typeid(Material); }

	virtual void Push(const CommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout) {}

	virtual void Inspect() {}
	virtual void Debug() { Inspect(); }

	static Material* Deserialize(const Scene::TValueMap& obj);

	static std::shared_ptr<Material> CreateDefault();

	virtual Workflow getWorkflow() const = 0;

	virtual void* getPushPointer() const = 0;

	size_t materialInstanceBufferOffset;
};

