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

	std::pair<int, CameraComponent*> makeKey() { return std::make_pair(priority, this); }

	Camera* camera() { return s_Camera.get(); }

private:
	friend class Scene;
	std::unique_ptr<Camera> s_Camera;
	int priority = 0;
};
