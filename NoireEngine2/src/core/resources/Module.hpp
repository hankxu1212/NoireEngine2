#pragma once

#include <bitset>
#include <functional>

#include "utils/Type.hpp"

template<typename Base>
class ModuleFactory {
public:
	class ModuleSpecification 
	{
	public:
		std::function<std::unique_ptr<Base>()> create;
		typename Base::UpdateStage stage;
		typename Base::DestroyStage destroyStage;
		std::vector<TypeId> requiredModules;
	};

	virtual ~ModuleFactory() = default;

	using RegistryMap = std::unordered_map<TypeId, ModuleSpecification>;

	static RegistryMap& GetRegistry() 
	{
		static RegistryMap r;
		return r;
	}

	template<typename ... Args>
	class Requires 
	{
	public:
		std::vector<TypeId> Get() const 
		{
			std::vector<TypeId> requiredModules;
			(requiredModules.emplace_back(Type<Base>::template GetTypeId<Args>()), ...);
			return requiredModules;
		}
	};

	template<typename T>
	class Registrar : public Base {
	public:
		virtual ~Registrar() 
		{
			if (static_cast<T*>(this) == moduleInstance)
				moduleInstance = nullptr;
		}

		// Gets the engines instance
		static T* Get() { return moduleInstance; }

	protected:
		/**
		  * Creates a new module singleton instance and registers into the module registry map.
		  * @tparam Args Modules that will be initialized before this module.
		  * @return A dummy value required in static initialization.
		*/
		template<typename ... Args>
		static bool Register(typename Base::UpdateStage stage, typename Base::DestroyStage destroyStage, Requires<Args...>&& requiredModules = {}) 
		{
			TypeId index = Type<Base>::template GetTypeId<T>();
			
			RegistryMap& registryMap = ModuleFactory::GetRegistry();

			registryMap[index] = {
				[]() {
					moduleInstance = new T();
					return std::unique_ptr<Base>(moduleInstance);
				}, stage, destroyStage, requiredModules.Get()
			};
			return true;
		}

		/**
		  * Creates a new module singleton instance and registers into the module registry map.
		  * @tparam Args Modules that will be initialized before this module.
		  * @return A dummy value in static initialization.
		  * Note: classes that use RegisterCreate() must implement static T* Create()
		*/
		template<typename ... Args>
		static bool RegisterCreate(typename Base::UpdateStage stage, typename Base::DestroyStage destroyStage, Requires<Args...>&& requiredModules = {}) 
		{
			TypeId index = Type<Base>::template GetTypeId<T>();

			RegistryMap& registryMap = ModuleFactory::GetRegistry();

			registryMap[index] = { 
				[]() {
					moduleInstance = T::Create();
					return std::unique_ptr<Base>(moduleInstance);
				}, stage, destroyStage, requiredModules.Get() 
			};
			return true;
		}
	protected:
		inline static T* moduleInstance = nullptr;
	};
};

// A interface used for defining engine modules.
class Module : public ModuleFactory<Module>, Singleton {
public:
	// Represents the stage where the module will be updated in the engine.
	enum class UpdateStage : uint8_t {
		Never, Always, Pre, Normal, Post, Render
	};

	/**
	  * @brief Represents the stage where the module will be destroyed in the engine.
	  * Note: dependencies will only be reinforced within the same destroy stage!
	*/
	enum class DestroyStage : uint8_t {
		Pre, Normal, Post
	};

	using StageIndex = std::pair<UpdateStage, TypeId>;

	virtual ~Module() = default;
	virtual void Update() = 0;
};

template class Type<Module>;