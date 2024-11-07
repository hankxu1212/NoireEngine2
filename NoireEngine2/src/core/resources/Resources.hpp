#pragma once

#include "Resource.hpp"
#include "nodes/Node.hpp"
#include "core/resources/Module.hpp"

/**
  * @brief Module used for managing resources. 
  * Resources are held alive as long as they are in use,
  * A existing resource is queried by node value.
*/
class Resources : public Module::Registrar<Resources>
{
	inline static const bool Registered = Register(UpdateStage::Post, DestroyStage::Normal);

public:
	Resources() = default;

	virtual ~Resources() = default;

	void Update() {}

public:
	using ResourceMap = std::map<Node, std::shared_ptr<Resource>>;
	using ResourceList = std::vector<std::shared_ptr<Resource>>;

	std::shared_ptr<Resource> Find(const std::type_index& typeIndex, const Node& node) const;

	// given a node, find a pointer to the node
	template<typename T>
	std::shared_ptr<T> Find(const Node& node) const 
	{
		if (resources.find(typeid(T)) == resources.end())
			return nullptr;

		for (const auto& [key, resource] : resources.at(typeid(T))) {
			if (key == node)
				return std::dynamic_pointer_cast<T>(resource);
		}

		return nullptr;
	}

	// find all resources of type T
	template<typename T>
	const ResourceList& FindAllOfType() const {
		return resourceIDs.at(typeid(T));
	}

	// add a resource
	void Add(const Node& node, const std::shared_ptr<Resource>& resource);

	void Remove(const std::shared_ptr<Resource>& resource);

private:
	template<typename T>
	void ReindexIDs() 
	{
		ResourceList& ids = FindAllOfType<T>();
		for (auto i = 0; i < ids.size(); i++)
		{
			ids[i]->ID = i;
		}
	}

	void ReindexIDs(std::type_index tid);

	// map of type index to resources
	std::unordered_map<std::type_index, ResourceMap> resources;

	// map of ID to resources
	std::unordered_map<std::type_index, ResourceList> resourceIDs;
};
