#pragma once

#include "GLFW/glfw3.h"

class Time
{
public:
	static double GetTime() { return glfwGetTime(); }
	inline static float DeltaTime = 0, Now = 0;
};
