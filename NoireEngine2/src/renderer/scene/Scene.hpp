#pragma once

#include "utils/Singleton.hpp"
#include "TransformMatrixStack.hpp"
#include "renderer/object/ObjectInstance.hpp"
#include "renderer/gizmos/GizmosInstance.hpp"
#include "utils/sejp/sejp.hpp"
#include "SceneNode.hpp"
#include "renderer/lighting/Light.hpp"
#include "backend/images/ImageCube.hpp"
#include "backend/images/Image2D.hpp"

#include <filesystem>
#include <unordered_map>
#include <set>

class Entity;
class Transform;
class CameraComponent;

#define MAX_NUM_TOTAL_LIGHTS 20
#define MAX_WORKFLOWS 4
#define GGX_MIP_LEVELS 6

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

	using TFoundInfo = std::pair<const std::optional<Scene::TValueMap>, const std::string&>;

	enum class CameraMode {
		Scene = 0, // all: sceneCam
		User = 1, // all: debugCam
		Debug = 2 // rendering: sceneCam, cull: sceneCam, move: debugCam
	};

public:
	Scene() = default;
	~Scene();

	void Load(const std::string& path);
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

	void PushObjectInstance(ObjectInstance&& instance, uint32_t index);

	void PushGizmosInstance(GizmosInstance* instance);

	CameraComponent* GetRenderCam() const;

	CameraComponent* GetCullingCam() const;

	inline CameraComponent* sceneCam() const;
	
	inline CameraComponent* debugCam() const;

	struct alignas(16) SceneUniform 
	{
		struct { float x, y, z, _padding; } cameraPosition;
		glm::uvec4 numLights;
		uint32_t shadowPCFSamples;
		uint32_t shadowOccluderSamples;
	};
	static_assert(sizeof(SceneUniform) == 16 * 3);

	inline const void* getSceneUniformPtr() const { return &m_SceneInfo; }

	inline size_t getSceneUniformSize() const { return sizeof(SceneUniform); }

	inline const std::vector<std::vector<ObjectInstance>>& getObjectInstances() const { return m_ObjectInstances; }

	inline const std::vector<GizmosInstance*>& getGizmosInstances() const { return m_GizmosInstances; }

	inline const std::vector<Light*>& getLightInstances() const { return m_SceneLights; }
	inline const std::vector<Light*>& getShadowInstances() const { return m_ShadowCasters; }

	inline const std::filesystem::path& getRootPath() const { return sceneRootAbsolutePath; }

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Skybox
	enum class SkyboxType { HDR, RGB };

	void AddSkybox(const std::string& path, SkyboxType type = SkyboxType::HDR, bool isDefault=false);

public: // event functions. Do not create function definitions!
	template<typename T>
	void OnComponentAdded(Entity&, T&);

	template<typename T>
	void OnComponentRemoved(Entity&, T&);

private:
	void InstantiateCoreScripts();
	void UpdateSceneInfo();
	void UpdateShadowCasters();

private:
	friend class SceneManager;
	friend class ObjectPipeline;

	std::filesystem::path sceneRootAbsolutePath;

	// a list of cameras, will be sorted everyframe ordered by their priority
	// in scene/debug mode, the scene will choose the smallest priority as the rendering/culling camera
	std::vector<CameraComponent*> m_SceneCameras;

	// in user mode, the scene will render everything through the debug camera
	CameraComponent* m_DebugCamera = nullptr;
	Core::SceneNavigationCamera* m_UserNavigationCamera = nullptr;

	TransformMatrixStack m_MatrixStack;

	// scene uiforms
	SceneUniform m_SceneInfo;

	std::vector<std::vector<ObjectInstance>> m_ObjectInstances;
	std::vector<GizmosInstance*> m_GizmosInstances;

	// a list of lights
	std::vector<Light*> m_SceneLights;
	std::vector<Light*> m_ShadowCasters;
	bool shadowCastersDirty = true;

	// IBL
	std::shared_ptr<ImageCube> m_Skybox;
	std::shared_ptr<ImageCube> m_SkyboxLambertian; // cosine weighted convolution on the skybox image
	std::shared_ptr<ImageCube> m_PrefilteredEnvMap;
	std::shared_ptr<Image2D> m_SpecularBRDF;
};