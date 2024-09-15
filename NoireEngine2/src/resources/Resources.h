#pragma once

#include "utils/ThreadPool.hpp"
#include "core/files/Files.h"
#include "Resource.h"
#include "Module.hpp"
#include "core/files/Node.hpp"
#include "core/Timer.h"
#include "core/Logger.h"

namespace Noire {
	/**
	 * @brief Module used for managing resources. Resources are held alive as long as they are in use,
	 * a existing resource is queried by node value.
	 */
	class NE_API Resources : public Module::Registrar<Resources> {
		inline static const bool Registered = Register(UpdateStage::Post, DestroyStage::Normal, Requires<Files>());
	public:
		Resources();

		~Resources();

		void Update() override;

		Ref<Resource> Find(const std::type_index& typeIndex, const Node& node) const;

		template<typename T>
		Ref<T> Find(const Node& node) const {
			if (resources.find(typeid(T)) == resources.end())
				return nullptr;

			for (const auto& [key, resource] : resources.at(typeid(T))) {
				if (key == node)
					return std::dynamic_pointer_cast<T>(resource);
			}

			return nullptr;
		}

		void Add(const Node& node, const Ref<Resource>& resource);
		void Remove(const Ref<Resource>& resource);

		/**
		 * Gets the resource loader thread pool.
		 * @return The resource loader thread pool.
		 */
		ThreadPool& GetThreadPool() { return threadPool; }

	private:
		UMap<std::type_index, Map<Node, Ref<Resource>>> resources;
		Timer elapsedPurge;

		ThreadPool threadPool;
	};
}
