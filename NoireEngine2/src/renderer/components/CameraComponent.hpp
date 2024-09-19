#pragma once

#include "Component.hpp"

#include "renderer/Camera.hpp"
#include <memory>

class CameraComponent : public Component
{
public:
	CameraComponent();
public:
	void Update() override;

	std::pair<int, Camera*> makeKey() { return std::make_pair(priority, s_Camera.get()); }

private:
	std::unique_ptr<Camera> s_Camera;
	int priority = 0;
};
