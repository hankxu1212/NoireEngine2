#include "Resources.h"
#include "core/Logger.h"

namespace Noire {
	Resources::Resources() :
		elapsedPurge(5s) {
	}

	Resources::~Resources()
	{
	}

	void Resources::Update() {
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

	Ref<Resource> Resources::Find(const std::type_index& typeIndex, const Node& node) const {
		if (resources.find(typeIndex) == resources.end())
			return nullptr;

		for (const auto& [key, resource] : resources.at(typeIndex)) {
			if (key == node)
				return resource;
		}

		return nullptr;
	}

	void Resources::Add(const Node& node, const Ref<Resource>& resource) {
		if (Find(resource->getTypeIndex(), node))
			return;

		resources[resource->getTypeIndex()].emplace(node, resource);
	}

	void Resources::Remove(const Ref<Resource>& resource) {
		auto& resources = this->resources[resource->getTypeIndex()];
		for (auto it = resources.begin(); it != resources.end(); ++it) { // TODO: Clean remove.
			if ((*it).second == resource)
				resources.erase(it);
		}
		if (resources.empty())
			this->resources.erase(resource->getTypeIndex());
	}
}

