#include "Camera.hpp"
#include "core/window/Window.hpp"
#include "Camera.hpp"

Camera::Camera()
{
	aspectRatio = (float)Window::Get()->m_Data.Width / (float)Window::Get()->m_Data.Height;
}

Camera::Camera(Type type_, bool orthographic_, float orthographicScale_, float np, float fp) :
	nearClipPlane(np), farClipPlane(fp), 
	orthographic(orthographic_), orthographicScale(orthographicScale_),
	type(type_) {
}

Camera::Camera(Type type_, bool orthographic_, float np, float fp, float fov, float aspect) :
	nearClipPlane(np), farClipPlane(fp),
	orthographic(orthographic_),
	type(type_),
	fieldOfView(fov),
	aspectRatio(aspect) {
}

void Camera::Update(const Transform& t)
{
	viewMatrix = glm::lookAt(t.m_Position, t.m_Position - t.Forward(), t.Up());

	if (orthographic) {
		projectionMatrix = glm::ortho(
			-aspectRatio * orthographicScale,
			aspectRatio * orthographicScale,
			-orthographicScale,
			orthographicScale,
			nearClipPlane, farClipPlane);
	}
	else
		projectionMatrix = glm::perspective(fieldOfView, aspectRatio, nearClipPlane, farClipPlane);

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