#include "Resources.hpp"

Resources::Resources() :
	elapsedPurge(5s) {
}

void Resources::Update() 
{
	if (elapsedPurge.GetElapsed() != 0) {
		for (auto it = resources.begin(); it != resources.end();) {
			for (auto it1 = it->second.begin(); it1 != it->second.end();) {
				if ((*it1).second.use_count() <= 1) {
					it1 = it->second.erase(it1);
					continue;
				}

				++it1;
			}

			if (it->second.empty()) {
				it = resources.erase(it);
				continue;
			}

			++it;
		}
	}
}

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
	if (Find(resource->getTypeIndex(), node))
		return;

	resources[resource->getTypeIndex()].emplace(node, resource);
}

void Resources::Remove(const std::shared_ptr<Resource>& resource) 
{
	auto& node_rsc_map = this->resources[resource->getTypeIndex()];
	
	for (auto it = node_rsc_map.begin(); it != node_rsc_map.end(); ++it) 
	{ // TODO: Clean remove.
		if ((*it).second == resource)
			node_rsc_map.erase(it);
	}

	if (node_rsc_map.empty())
		this->resources.erase(resource->getTypeIndex());
}

