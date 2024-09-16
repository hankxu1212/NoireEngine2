#include "Math.hpp"
#include <random>

static std::random_device RandomDevice;
static std::mt19937 RandomGenerator(RandomDevice());

float Math::Random(float min, float max) {
	std::uniform_real_distribution<float> dist(min, max);
	return dist(RandomGenerator);
}

float Math::RandomNormal(float standardDeviation, float mean) {
	std::normal_distribution<float> dist(mean, standardDeviation);
	return dist(RandomGenerator);
}

float Math::RandomLog(float min, float max) {
	auto logLower = std::log(min);
	auto logUpper = std::log(max);
	auto raw = Random(0.0f, 1.0f);

	auto result = std::exp(raw * (logUpper - logLower) + logLower);

	if (result < min) {
		result = min;
	}
	else if (result > max) {
		result = max;
	}

	return result;
}
