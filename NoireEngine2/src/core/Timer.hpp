#pragma once

#include <chrono>
#include "core/Core.hpp"

using namespace std::chrono_literals;

class Timer
{
public:
	Timer() 
	{
		Reset();
	}

	float GetElapsed(bool reset) 
	{
		float elasped = float(std::chrono::duration<double, std::milli>(TIME_NOW - start).count());
		if (reset) Reset();
		return elasped;
	}

	void Reset()
	{
		start = TIME_NOW;
	}

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> start;
};