#include "NativeInput.hpp"

#include "GLFW/glfw3.h"
#include "core/window/Window.hpp"
#include "math/Math.hpp"

bool NativeInput::IsKeyPressed(const KeyCode key)
{
	auto state = glfwGetKey(Window::Get().m_Window, static_cast<int32_t>(key));
	return state == GLFW_PRESS;
}

bool NativeInput::IsMouseButtonPressed(const MouseCode button)
{
	auto state = glfwGetMouseButton(Window::Get().m_Window, static_cast<int32_t>(button));
	return state == GLFW_PRESS;
}

glm::vec2 NativeInput::GetMousePosition()
{
	double xpos, ypos;
	glfwGetCursorPos(Window::Get().m_Window, &xpos, &ypos);

	return { (float)xpos, (float)ypos };
}

float NativeInput::GetVec1Input(InputTypeVec1 inputKeys)
{
	float acc = 0;

	if (NativeInput::IsKeyPressed(inputKeys.forward))
		acc++;
	if (NativeInput::IsKeyPressed(inputKeys.backward))
		acc--;

	return acc;
}

glm::vec2 NativeInput::GetVec2Input(InputTypeVec2 inputKeys)
{
	glm::vec2 acc = { 0,0 };

	if (NativeInput::IsKeyPressed(inputKeys.forward))
		acc += glm::vec2({ 1, 0 });
	if (NativeInput::IsKeyPressed(inputKeys.backward))
		acc += glm::vec2({ -1, 0 });
	if (NativeInput::IsKeyPressed(inputKeys.left))
		acc += glm::vec2({ 0, 1 });
	if (NativeInput::IsKeyPressed(inputKeys.right))
		acc += glm::vec2({ 0, -1 });

	if (acc != glm::vec2())
		acc = glm::normalize(acc);

	return acc;
}

glm::vec3 NativeInput::GetVec3Input(InputTypeVec3& inputKeys)
{
	glm::vec3 acc = { 0,0,0 };

	if (NativeInput::IsKeyPressed(inputKeys.forward))
		acc += Vec3::Forward;
	if (NativeInput::IsKeyPressed(inputKeys.backward))
		acc += Vec3::Back;
	if (NativeInput::IsKeyPressed(inputKeys.left))
		acc += Vec3::Left;
	if (NativeInput::IsKeyPressed(inputKeys.right))
		acc += Vec3::Right;
	if (NativeInput::IsKeyPressed(inputKeys.up))
		acc += Vec3::Up;
	if (NativeInput::IsKeyPressed(inputKeys.down))
		acc += Vec3::Down;

	if (acc != Vec3::Zero)
		acc = glm::normalize(acc);

	return acc;
}