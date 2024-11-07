#include "Resources.hpp"

#include <iostream>

std::shared_ptr<Resource> Resources::Find(const std::type_index& typeIndex, const Node& node) const 
{
	if (resources.find(typeIndex) == resources.end())
		return nullptr;

	for (const auto& [key, resource] : resources.at(typeIndex)) {
		if (key == node)
			return resource;
	}

	return nullptr;
}

void Resources::Add(const Node& node, const std::shared_ptr<Resource>& resource)
{
	std::type_index tid = resource->getTypeIndex();
	if (Find(tid, node))
		return;

	resources[tid].emplace(node, resource);
	resourceIDs[tid].emplace_back(resource);

	resource->ID = (uint32_t)resourceIDs[tid].size() - 1;
}

void Resources::Remove(const std::shared_ptr<Resource>& resource) 
{
	std::type_index tid = resource->getTypeIndex();

	ResourceMap& node_rsc_map = resources[tid];
	std::erase_if(node_rsc_map, [&](const auto& pair) {
		return pair.second == resource;
	});

	if (node_rsc_map.empty())
		resources.erase(tid);

	ReindexIDs(tid);
}

void Resources::ReindexIDs(std::type_index tid)
{
	ResourceList& ids = resourceIDs.at(tid);
	for (auto i = 0; i < ids.size(); i++)
	{
		ids[i]->ID = i;
	}
}
