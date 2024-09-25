#pragma once

#include "Component.hpp"

#include "renderer/Camera.hpp"
#include <memory>

class CameraComponent : public Component
{
public:
	CameraComponent(int priority_);

	template<typename... TArgs>
	CameraComponent(TArgs... args) :
		s_Camera(std::make_unique<Camera>(args...)) {
	}

public:
	void Update() override;

	Camera* camera() { return s_Camera.get(); }

	void Inspect() override;

	void Debug() override;

	const char* getName() override { return "Camera"; }

private:
	friend class Scene;
	std::unique_ptr<Camera> s_Camera;
	int priority = 0;
};
