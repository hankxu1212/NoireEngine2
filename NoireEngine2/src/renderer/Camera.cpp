#include "Camera.hpp"
#include "core/window/Window.hpp"
#include "Camera.hpp"
#include "utils/Logger.hpp"

Camera::Camera()
{
	aspectRatio = (float)Window::Get()->m_Data.Width / (float)Window::Get()->m_Data.Height;
}

Camera::Camera(Type type_) :
	type(type_) {
	aspectRatio = (float)Window::Get()->m_Data.Width / (float)Window::Get()->m_Data.Height;
}

Camera::Camera(Type type_, bool orthographic_, float orthographicScale_, float np, float fp) :
	nearClipPlane(np), farClipPlane(fp), 
	orthographic(orthographic_), orthographicScale(orthographicScale_),
	type(type_) {
	aspectRatio = (float)Window::Get()->m_Data.Width / (float)Window::Get()->m_Data.Height;
}

Camera::Camera(Type type_, bool orthographic_, float np, float fp, float fov, float aspect) :
	nearClipPlane(np), farClipPlane(fp),
	orthographic(orthographic_),
	type(type_),
	fieldOfView(fov),
	aspectRatio(aspect) {
}

void Camera::Update(const Transform& t, bool updateFrustum)
{
	if (!isDirty) {
		wasDirtyThisFrame = false;
		return;
	}

	glm::vec3 eye = t.position();
	glm::vec3 target = eye + t.Back();

	viewMatrix = Mat4::LookAt(eye, target, t.Up());

	if (orthographic) {
		projectionMatrix = glm::ortho(
			-aspectRatio * orthographicScale * 0.5f,
			aspectRatio * orthographicScale * 0.5f,
			-orthographicScale * 0.5f,
			orthographicScale * 0.5f,
			nearClipPlane, farClipPlane);
	}
	else
		projectionMatrix = Mat4::Perspective(fieldOfView, aspectRatio, nearClipPlane, farClipPlane);

	if (updateFrustum)
		frustum.Update(viewMatrix, projectionMatrix);

	worldToClip = projectionMatrix * viewMatrix;

	isDirty = false;
	wasDirtyThisFrame = true;
}

const char* Camera::getTypeStr() const
{
	switch (type)
	{
	case Type::Scene:
		return "Scene";
	case Type::Game:
		return "Game";
	case Type::Debug:
		return "Debug";
	case Type::Other:
		return "Other";
	default:
		return "ERROR: <NO TYPE FOUND>";
	}
}