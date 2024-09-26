#pragma once

#include <vector>
#include <string>
#include <bitset>

#include "Keyframe.hpp"
#include "core/resources/Resource.hpp"

class Animation : public Resource
{
public:
	std::vector<Keyframe> keyframes;
	std::string name;
	float duration;

	enum class Channel {
		Position = 0b00, 
		Rotation = 0b01, 
		Scale = 0b10
	};

	std::bitset<2> channels;

	std::type_index getTypeIndex() const { return typeid(Resource); }
};

