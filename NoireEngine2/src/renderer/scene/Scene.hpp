#pragma once

#include "utils/Singleton.hpp"
#include "TransformMatrixStack.hpp"
#include "renderer/object/ObjectInstance.hpp"
#include "utils/sejp/sejp.hpp"
#include "SceneNode.hpp"

#include <filesystem>
#include <unordered_map>
#include <set>

class Entity;
class CameraComponent;
class Transform;

class Scene : Singleton
{
public:
	// { name, serialized object map }
	using TValueMap = std::map<std::string, sejp::value>;

	// { name, serialized object unordered map }
	using TValueUMap = std::unordered_map<std::string, sejp::value>;

	// Maps from object type to serialized object maps
	using TSceneMap = std::unordered_map<SceneNode::Type, TValueUMap>;

public:
	Scene();
	Scene(const std::string& path);
	~Scene();

	void Unload();

	void Update();

	void Deserialize(const std::string& path);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Entity management

	template<typename... TArgs>
	Entity* Instantiate(TArgs&... args) { return Entity::root().AddChild(this, args...); }

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Rendering and scene uniforms

	void PushObjectInstances(const ObjectInstance&& instance);

	inline CameraComponent* mainCam() const;

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

public: // event functions. Do not create function definitions!
	template<typename T>
	void OnComponentAdded(Entity&, T&);

	template<typename T>
	void OnComponentRemoved(Entity&, T&);

private:
	void UpdateWorldUniform();

private:
	// a list of cameras with their priority as min heap key
	std::set<std::pair<int, CameraComponent*>> m_Cameras;
	TransformMatrixStack m_MatrixStack;

	//types for descriptors:
	SceneUniform m_SceneInfo;

	std::vector<ObjectInstance> m_ObjectInstances;
};