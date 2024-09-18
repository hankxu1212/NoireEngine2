#pragma once

#include "Entity.hpp"
#include "utils/Singleton.hpp"

#include <filesystem>
#include <set>

class Camera;

class Scene : Singleton
{
	static Entity root;

public:
	Scene();
	Scene(std::filesystem::path& path);

	void Update();

private:
	// a list of cameras with their priority as min heap key
	std::set<std::pair<int, Camera*>> m_Cameras;
};

