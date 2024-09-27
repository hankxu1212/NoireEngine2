#pragma once

#include "utils/Singleton.hpp"
#include "TransformMatrixStack.hpp"
#include "renderer/object/ObjectInstance.hpp"
#include "utils/sejp/sejp.hpp"
#include "SceneNode.hpp"
#include "renderer/lighting/Light.hpp"

#include <filesystem>
#include <unordered_map>
#include <set>

class Entity;
class Transform;
class CameraComponent;

#define MAX_NUM_TOTAL_LIGHTS 20

namespace Core {
	class SceneNavigationCamera;
}

class Scene : Singleton
{
public:
	// { name, serialized object map }
	using TValueMap = std::map<std::string, sejp::value>;

	// { name, serialized object unordered map }
	using TValueUMap = std::unordered_map<std::string, sejp::value>;

	// Maps from object type to serialized object maps
	using TSceneMap = std::unordered_map<SceneNode::Type, TValueUMap>;

	enum class CameraMode {
		Scene = 0, // all: sceneCam
		User = 1, // all: debugCam
		Debug = 2 // rendering: sceneCam, cull: sceneCam, move: debugCam
	};

public:
	Scene(const std::string& path);
	~Scene();

	void Unload();

	void Update();

	void Render();

	void Deserialize(const std::string& path);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Entity management

	template<typename... TArgs>
	Entity* Instantiate(TArgs&... args) { return Entity::root().AddChild(this, args...); }

	template<typename... TArgs>
	Entity* Instantiate(TArgs&&... args) { return Entity::root().AddChild(this, args...); }

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Rendering and scene uniforms

	void PushObjectInstance(ObjectInstance&& instance);

	CameraComponent* GetRenderCam();

	CameraComponent* GetCullingCam();

	inline CameraComponent* sceneCam();
	
	inline CameraComponent* debugCam() const;

	struct SceneUniform {
		LightUniform lights[MAX_NUM_TOTAL_LIGHTS];
		uint32_t numLights;
	};

	inline const void* getSceneUniformPtr() const { return &m_SceneInfo; }

	inline size_t getSceneUniformSize() const { return sizeof(SceneUniform); }

	inline const std::vector<ObjectInstance>& getObjectInstances() const { return m_ObjectInstances; }

	inline const std::vector<Light*>& getLightInstances() const { return m_SceneLights; }

public: // event functions. Do not create function definitions!
	template<typename T>
	void OnComponentAdded(Entity&, T&);

	template<typename T>
	void OnComponentRemoved(Entity&, T&);

private:
	void InstantiateCoreScripts();
	void UpdateSceneInfo();

private:
	friend class SceneManager;

	// a list of cameras, will be sorted everyframe ordered by their priority
	// in scene/debug mode, the scene will choose the smallest priority as the rendering/culling camera
	std::vector<CameraComponent*> m_SceneCameras;
	bool sceneCamDirty = true;

	// in user mode, the scene will render everything through the debug camera
	CameraComponent* m_DebugCamera = nullptr;
	Core::SceneNavigationCamera* m_UserNavigationCamera = nullptr;

	TransformMatrixStack m_MatrixStack;

	//types for descriptors:
	SceneUniform m_SceneInfo;

	std::vector<ObjectInstance> m_ObjectInstances;

	// a list of lights
	std::vector<Light*> m_SceneLights;
};