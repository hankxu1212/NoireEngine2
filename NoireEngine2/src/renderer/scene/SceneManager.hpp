#pragma once

#include "core/resources/Module.hpp"
#include "backend/VulkanContext.hpp"
#include "Scene.hpp"

/**
 * Holds and updates the unique pointer to the active scene.
 */
class SceneManager : public Module::Registrar<SceneManager>
{
	inline static const bool Registered = Register(UpdateStage::Pre, DestroyStage::Normal, Requires<VulkanContext>());
public:
	SceneManager();
	~SceneManager();

	/**
	 * Updates the active scene.
	 */
	void Update();

	void Shutdown();

	std::unique_ptr<Scene> scene; // the active scene
};
