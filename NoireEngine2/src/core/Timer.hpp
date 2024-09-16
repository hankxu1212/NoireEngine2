#pragma once

#include <chrono>

using namespace std::chrono_literals;

class Timer
{
public:
	Timer(const std::chrono::duration<double, std::milli>& duration) {
		m_Start = std::chrono::high_resolution_clock::now();
		m_Interval = duration;
	}

	uint32_t GetElapsed() {
		auto now = std::chrono::high_resolution_clock::now();
		auto elapsed = static_cast<uint32_t>(std::floor((now - m_Start) / m_Interval));

		if (elapsed != 0.0f) {
			m_Start = now;
		}

		return elapsed;
	}

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;

	std::chrono::duration<double, std::milli> m_Interval;
};