#pragma once

#include <vector>
#include <string>
#include <bitset>

#include "Keyframe.hpp"
#include "core/resources/Resources.hpp"
#include "renderer/scene/Scene.hpp"

class Animation : public Resource
{
public:
	Animation() = default;
	Animation(const std::string& name, const std::string& filename);


	enum class Channel {
		Position = 0b00, 
		Rotation = 0b01, 
		Scale = 0b10
	};

	enum class Interpolation {
		Lerp, Slerp, Constant
	};

	static std::shared_ptr<Animation> Create(const std::string& name, const std::string& filename);
	static std::shared_ptr<Animation> Create(const Node& node);

	void Load();
	void Load(const Scene::TValueMap& obj);

	friend const Node& operator>>(const Node& node, Animation& anim);
	friend Node& operator<<(Node& node, const Animation& anim);

	std::type_index getTypeIndex() const { return typeid(Resource); }

	std::string				m_Name;
	std::string				m_Filename;
	Interpolation			m_InterpolationMode = Interpolation::Slerp;
	std::bitset<3>			m_Channels;

	std::vector<Keyframe> keyframes;
	float duration;
};


