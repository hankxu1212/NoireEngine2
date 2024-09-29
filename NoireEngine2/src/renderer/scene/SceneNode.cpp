#include "SceneNode.hpp"

#include <unordered_map>
#include <iostream>
#include <format>

const static std::unordered_map<std::string, SceneNode::Type> strToType
{
	{"CAMERA",		SceneNode::Camera},
	{"NODE",		SceneNode::Node},
	{"MATERIAL",	SceneNode::Material},
	{"MESH",		SceneNode::Mesh},
	{"DRIVER",		SceneNode::Driver},
	{"SCENE",		SceneNode::Scene},
	{"ENVIRONMENT", SceneNode::Environment},
	{"LIGHT",		SceneNode::Light},
};

SceneNode::Type SceneNode::ObjectType(const std::string& key)
{
	auto it = strToType.find(key);
	if (it != strToType.end()) {
		return it->second;
	}
	else { 
		throw std::runtime_error(std::format("Could not find this key: {}", key));
	}
}