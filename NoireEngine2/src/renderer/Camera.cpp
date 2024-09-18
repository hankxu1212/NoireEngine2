#include "Camera.hpp"
#include "core/window/Window.hpp"

Camera::Camera()
{
	screenWidth = (float)Window::Get().m_Data.Width;
	screenHeight = (float)Window::Get().m_Data.Height;
	aspectRatio = screenWidth / screenHeight;
}

Camera::Camera(Type type_, bool orthographic_, float orthographicScale_, float np, float fp)
	: type(type_), orthographic(orthographic_), orthographicScale(orthographicScale_),
	nearClipPlane(np), farClipPlane(fp) {
}

void Camera::Update(const Transform& t)
{
	viewMatrix = glm::lookAt(t.position, t.position - t.Forward(), t.Up());
	aspectRatio = screenWidth / screenHeight;

	if (orthographic) {
		projectionMatrix = glm::ortho(
			-aspectRatio * orthographicScale,
			aspectRatio * orthographicScale,
			-orthographicScale,
			orthographicScale,
			nearClipPlane, farClipPlane);
	}
	else
		projectionMatrix = glm::perspective(glm::radians(fieldOfView), aspectRatio, nearClipPlane, farClipPlane);

	projectionMatrix[1][1] *= -1;

	//frustum.Update(viewMatrix, projectionMatrix);

	position = t.WorldLocation();
}

const char* Camera::getTypeStr() const
{
	switch (type)
	{
	case Type::Scene:
		return "Scene";
	case Type::Preview:
		return "Preview";
	case Type::Reflection:
		return "Reflection";
	case Type::Game:
		return "Game";
	case Type::Light:
		return "Light";
	default:
		return "ERROR: <NO TYPE FOUND>";
	}
}
