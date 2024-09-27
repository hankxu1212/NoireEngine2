#include "Animation.hpp"
#include "utils/Logger.hpp"
#include <iostream>

Animation::Animation(const std::string& name, const std::string& filename) :
	m_Name(name), m_Filename(filename)
{
	m_Channels.reset();
}

std::shared_ptr<Animation> Animation::Create(const std::string& name, const std::string& filename)
{
	Animation temp(name, filename);
	Node node;
	node << temp;
	return Create(node);
}

std::shared_ptr<Animation> Animation::Create(const Node& node)
{
	if (auto resource = Resources::Get()->Find<Animation>(node)) {
		NE_INFO("Reusing old animation from {}", resource->m_Filename);
		return resource;
	}

	auto result = std::make_shared<Animation>();
	Resources::Get()->Add(node, std::dynamic_pointer_cast<Resource>(result));
	node >> *result;
	result->Load();
	return result;
}

void Animation::Load()
{
}

void Animation::Load(const Scene::TValueMap& obj)
{
	try {
		// channels
		const auto& channel = obj.at("channel").as_string().value();
		if (channel == "rotation")
			m_Channels.set((uint8_t)Channel::Rotation);
		else if (channel == "translation") // TODO: verify this is translation and not position
			m_Channels.set((uint8_t)Channel::Position);
		else if (channel == "scale")
			m_Channels.set((uint8_t)Channel::Scale);
		else
			NE_WARN("Did not find this channel: " + channel);

		// interporlation
		const auto& lerp = obj.at("interpolation").as_string().value();
		if (lerp == "SLERP")
			m_InterpolationMode = Interpolation::Slerp;
		else if (lerp == "LERP")
			m_InterpolationMode = Interpolation::Lerp;
		else if (lerp == "LERP")
			m_InterpolationMode = Interpolation::Constant;
		else
			NE_WARN("Did not find this interpolation mode: " + lerp);

		// times
		keyframes.clear();
		const auto& times = obj.at("times").as_array().value();
		const auto& values = obj.at("values").as_array().value();

		if (m_Channels.test((uint8_t)Channel::Rotation))
		{
			for (uint32_t i = 0; i < times.size(); ++i)
			{
				float ts = times[i].as_float();

				duration = std::max(duration, ts);

				glm::quat rot;
				rot.x = values[i * 4].as_float();
				rot.y = values[i * 4 + 1].as_float();
				rot.z = values[i * 4 + 2].as_float();
				rot.w = values[i * 4 + 3].as_float();

				keyframes.emplace_back(ts, rot);
			}
		}
		if (m_Channels.test((uint8_t)Channel::Position))
		{
			for (uint32_t i = 0; i < times.size(); ++i)
			{
				float ts = times[i].as_float();

				duration = std::max(duration, ts);

				glm::vec3 v;
				v.x = values[i * 3].as_float();
				v.y = values[i * 3 + 1].as_float();
				v.z = values[i * 3 + 2].as_float();

				keyframes.emplace_back(ts, v);
			}
		}
		if (m_Channels.test((uint8_t)Channel::Scale))
		{
			for (uint32_t i = 0; i < times.size(); ++i)
			{
				float ts = times[i].as_float();

				duration = std::max(duration, ts);

				glm::vec3 v;
				v.x = values[i * 3].as_float();
				v.y = values[i * 3 + 1].as_float();
				v.z = values[i * 3 + 2].as_float();

				Keyframe kf;
				kf.timestamp = ts;
				kf.scale = v;

				keyframes.emplace_back(kf);
			}
		}
	}
	catch (std::exception & e) {
		NE_WARN("Could not load animation file {} ", e.what());
	}
}

const Node& operator>>(const Node& node, Animation& anim)
{
	node["name"].Get(anim.m_Name);
	node["filename"].Get(anim.m_Filename);
	return node;
}

Node& operator<<(Node& node, const Animation& anim)
{
	node["name"].Set(anim.m_Name);
	node["filename"].Set(anim.m_Filename);
	return node;
}
