#pragma once

#include "core/resources/Resources.hpp"

#include <vulkan/vulkan.h>
#include "backend/commands/CommandBuffer.hpp"
#include "renderer/scene/Scene.hpp"
#include "core/Core.hpp"

class Material : public Resource
{
public:
	enum class Workflow
	{
		Lambertian,
		PBR,
		Glass
	};

public:
	virtual std::type_index getTypeIndex() const { return typeid(Material); }

	virtual void Inspect() {}
	virtual void Debug() { Inspect(); }

	static Material* Deserialize(const Scene::TValueMap& obj);

	static std::shared_ptr<Material> CreateDefault();

	virtual void Load() = 0;

	virtual Workflow getWorkflow() const = 0;

	virtual void* getPushPointer() const = 0;

	size_t materialInstanceBufferOffset;

	template<typename T, typename CreateInfo>
	static std::shared_ptr<Material> Create()
	{
		CreateInfo defaultInfo;
		return Create<T>(defaultInfo);
	}

	template<typename T, typename CreateInfo>
	static std::shared_ptr<Material> Create(const CreateInfo& createInfo)
	{
		T temp(createInfo);
		Node node;
		node << temp;
		return Create<T>(node);
	}

private:
	template<typename T>
	static std::shared_ptr<Material> Create(const Node& node)
	{
		if (auto resource = Resources::Get()->Find<Material>(node)) {
			return resource;
		}

		auto result = std::make_shared<T>();
		Resources::Get()->Add(node, std::dynamic_pointer_cast<Resource>(result));
		node >> *result;
		result->Load();
		return result;
	}
};

