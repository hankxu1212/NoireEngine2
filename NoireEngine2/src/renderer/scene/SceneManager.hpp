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

	void LoadDefault();

	/**
	 * Updates the active scene.
	 */
	void Update();

	void Shutdown();

	/**
	 * Gets the current scene.
	 * \return The current scene.
	 */
	inline Scene* getScene() const { return scene.get(); }

	void SetCameraMode(Scene::CameraMode newMode);
	Scene::CameraMode getCameraMode() const { return cameraMode; }

private:
	Scene::CameraMode cameraMode = Scene::CameraMode::User;
	std::unique_ptr<Scene> scene; // the active scene
};

#define VIEW_CAM SceneManager::Get()->getScene()->GetRenderCam()->camera()