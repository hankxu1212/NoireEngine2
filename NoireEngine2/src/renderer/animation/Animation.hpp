#pragma once

#include <vector>
#include <string>
#include "Keyframe.hpp"
#include <bitset>

struct Animation
{
	std::vector<Keyframe> keyframes;
	std::string name;
	float duration;

	enum class Channel {
		Position = 0b00, 
		Rotation = 0b01, 
		Scale = 0b10
	};

	std::bitset<2> channels;
};

