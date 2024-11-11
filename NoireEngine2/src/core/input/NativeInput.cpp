#include "NativeInput.hpp"

#include "GLFW/glfw3.h"
#include "core/window/Window.hpp"
#include "math/Math.hpp"

bool NativeInput::GetKeyPressed(const KeyCode key)
{
	auto state = glfwGetKey(Window::Get()->nativeWindow, static_cast<int32_t>(key));
	return state == GLFW_PRESS;
}

bool NativeInput::GetMouseButtonPressed(const MouseCode button)
{
	auto state = glfwGetMouseButton(Window::Get()->nativeWindow, static_cast<int32_t>(button));
	return state == GLFW_PRESS;
}

bool NativeInput::GetKeyRepeat(KeyCode key)
{
	auto state = glfwGetKey(Window::Get()->nativeWindow, static_cast<int32_t>(key));
	return state == GLFW_REPEAT;
}

bool NativeInput::GetMouseButtonRepeat(MouseCode button)
{
	auto state = glfwGetMouseButton(Window::Get()->nativeWindow, static_cast<int32_t>(button));
	return state == GLFW_REPEAT;
}

bool NativeInput::GetKeyRelease(KeyCode key)
{
	auto state = glfwGetKey(Window::Get()->nativeWindow, static_cast<int32_t>(key));
	return state == GLFW_RELEASE;
}

bool NativeInput::GetMouseButtonRelease(MouseCode button)
{
	auto state = glfwGetMouseButton(Window::Get()->nativeWindow, static_cast<int32_t>(button));
	return state == GLFW_RELEASE;
}

glm::vec2 NativeInput::GetMousePosition()
{
	double xpos, ypos;
	glfwGetCursorPos(Window::Get()->nativeWindow, &xpos, &ypos);

	return { (float)xpos, (float)ypos };
}

float NativeInput::GetVec1Input(InputTypeVec1 inputKeys)
{
	float acc = 0;

	if (NativeInput::GetKeyPressed(inputKeys.forward))
		acc++;
	if (NativeInput::GetKeyPressed(inputKeys.backward))
		acc--;

	return acc;
}

glm::vec2 NativeInput::GetVec2Input(InputTypeVec2 inputKeys)
{
	glm::vec2 acc = { 0,0 };

	if (NativeInput::GetKeyPressed(inputKeys.forward))
		acc += glm::vec2({ 1, 0 });
	if (NativeInput::GetKeyPressed(inputKeys.backward))
		acc += glm::vec2({ -1, 0 });
	if (NativeInput::GetKeyPressed(inputKeys.left))
		acc += glm::vec2({ 0, 1 });
	if (NativeInput::GetKeyPressed(inputKeys.right))
		acc += glm::vec2({ 0, -1 });

	if (acc != glm::vec2())
		acc = normalize(acc);

	return acc;
}

glm::vec3 NativeInput::GetVec3Input(InputTypeVec3& inputKeys)
{
	glm::vec3 acc = { 0,0,0 };

	if (NativeInput::GetKeyPressed(inputKeys.forward))
		acc += Vec3::Forward;
	if (NativeInput::GetKeyPressed(inputKeys.backward))
		acc += Vec3::Back;
	if (NativeInput::GetKeyPressed(inputKeys.left))
		acc += Vec3::Left;
	if (NativeInput::GetKeyPressed(inputKeys.right))
		acc += Vec3::Right;
	if (NativeInput::GetKeyPressed(inputKeys.up))
		acc += Vec3::Up;
	if (NativeInput::GetKeyPressed(inputKeys.down))
		acc += Vec3::Down;

	if (acc != Vec3::Zero)
		acc = normalize(acc);

	return acc;
}