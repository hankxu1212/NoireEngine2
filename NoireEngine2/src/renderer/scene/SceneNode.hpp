#pragma once

#include <string>

class SceneNode;


class SceneNode 
{
public:
	enum Type {
		Camera, Node, Material, Mesh, Driver, Scene, Environment, Light
	};

	static Type ObjectType(const std::string& key);
};