#pragma once

#include "Entity.hpp"
#include "utils/Singleton.hpp"
#include "TransformMatrixStack.hpp"

#include <filesystem>
#include <set>


class Camera;

class Scene : Singleton
{
public:
	Scene();
	Scene(std::filesystem::path& path);

	void Unload();

	void Update();

	template<typename... TArgs>
	Entity* Instantiate(TArgs&... args)
	{
		return Entity::root().AddChild(args...);
	}

private:
	// a list of cameras with their priority as min heap key
	std::set<std::pair<int, Camera*>> m_Cameras;
	TransformMatrixStack m_MatrixStack;

	//types for descriptors:
	struct SceneUniform {
		struct { float x, y, z, padding_; } SKY_DIRECTION;
		struct { float r, g, b, padding_; } SKY_ENERGY;
		struct { float x, y, z, padding_; } SUN_DIRECTION;
		struct { float r, g, b, padding_; } SUN_ENERGY;
	} m_SceneInfo;

	static_assert(sizeof(SceneUniform) == 16 * 4, "World is the expected size.");
};

