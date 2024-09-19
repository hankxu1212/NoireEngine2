#pragma once

#include "Entity.hpp"
#include "utils/Singleton.hpp"
#include "TransformMatrixStack.hpp"
#include "renderer/object/ObjectInstance.hpp"

#include <filesystem>
#include <set>


class Camera;

class Scene : Singleton
{
public:
	Scene();
	Scene(std::filesystem::path& path);
	~Scene();

	void Unload();

	void Update();


	template<typename... TArgs>
	Entity* Instantiate(TArgs&... args)
	{
		return Entity::root().AddChild(this, args...);
	}


	void PushObjectInstances(const ObjectInstance&& instance);

	inline Camera* mainCam() const;

	struct SceneUniform {
		struct { float x, y, z, padding_; } SKY_DIRECTION;
		struct { float r, g, b, padding_; } SKY_ENERGY;
		struct { float x, y, z, padding_; } SUN_DIRECTION;
		struct { float r, g, b, padding_; } SUN_ENERGY;
	};
	static_assert(sizeof(SceneUniform) == 16 * 4, "World is the expected size.");


	inline const void* sceneUniform() const { return &m_SceneInfo; }
	inline size_t sceneUniformSize() const { return sizeof(m_SceneInfo); }


	inline const std::vector<ObjectInstance>& objectInstances() const { return m_ObjectInstances; }

private:
	void UpdateWorldUniform();

private:
	// a list of cameras with their priority as min heap key
	std::set<std::pair<int, Camera*>> m_Cameras;
	TransformMatrixStack m_MatrixStack;

	//types for descriptors:
	SceneUniform m_SceneInfo;

	std::vector<ObjectInstance> m_ObjectInstances;
};