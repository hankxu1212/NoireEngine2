#pragma once

#include "utils/ThreadPool.hpp"
#include "Resource.hpp"
#include "nodes/Node.hpp"
#include "core/Timer.hpp"
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
	std::shared_ptr<Resource> Find(const std::type_index& typeIndex, const Node& node) const;

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

	void Add(const Node& node, const std::shared_ptr<Resource>& resource);

	void Remove(const std::shared_ptr<Resource>& resource);

	/**
		* Gets the resource loader thread pool.
		* @return The resource loader thread pool.
		*/
	ThreadPool& GetThreadPool() { return threadPool; }

private:
	std::unordered_map<std::type_index, std::map<Node, std::shared_ptr<Resource>>> resources;
	ThreadPool threadPool;
};
