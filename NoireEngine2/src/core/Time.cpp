#include "Time.hpp"

#include <GLFW/glfw3.h>

double Time::GetTime()
{
	return glfwGetTime();
}
