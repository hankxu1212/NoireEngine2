#include "renderer/components/Components.hpp"
#include "core/Time.hpp"
#include "Entity.hpp"
#include "core/resources/Files.hpp"
#include "utils/Enumerate.hpp"
#include "utils/Logger.hpp"
#include "SceneManager.hpp"
#include "renderer/object/Mesh.hpp"

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <algorithm>

Scene::Scene()
{
}

Scene::Scene(const std::string& path)
{
	Deserialize(path);
	UpdateWorldUniform();
}

Scene::~Scene()
{
	Unload();
}

void Scene::Unload()
{
}

void Scene::Update()
{
	// TODO: update lightings
	//UpdateWorldUniform();

	// update entities and components
	Entity::root().Update();
}

void Scene::Render()
{
	if (GetRenderCam() == nullptr)
	{
		NE_ERROR("Render camera is null");
		return;
	}

	// push all transforms to pipeline
	m_MatrixStack.Clear();
	m_ObjectInstances.clear();

	for (auto& child : Entity::root().children()) 
	{
		assert(m_MatrixStack.Size() == 0);
		child->RenderPass(m_MatrixStack);
 	}
}

static bool LoadAsTransform(const sejp::value& val, glm::vec3& outPosition, glm::quat& outRotation, glm::vec3& outScale)
{
	try {
		const auto& obj = val.as_object().value();

		const auto& translation = obj.at("translation").as_array().value();
		assert(translation.size() == 3);

		const auto& rotation = obj.at("rotation").as_array().value();
		assert(rotation.size() == 4);

		const auto& scale = obj.at("scale").as_array().value();
		assert(scale.size() == 3);

		outPosition.x = translation[0].as_float();
		outPosition.y = translation[1].as_float();
		outPosition.z = translation[2].as_float();

		outRotation.x = rotation[0].as_float();
		outRotation.y = rotation[1].as_float();
		outRotation.z = rotation[2].as_float();
		outRotation.w = rotation[3].as_float();

		outScale.x = scale[0].as_float();
		outScale.y = scale[1].as_float();
		outScale.z = scale[2].as_float();

		return true;
	}
	catch (std::exception& e) {
		std::cout << "Failed to deserialize value as Transform: " << e.what() << std::endl;

		return false;
	}
}

static void MakeMesh(Entity* newEntity, const Scene::TValueMap& obj, const Scene::TSceneMap& sceneMap)
{
	if (obj.find("mesh") == obj.end())
		return;

	const auto& meshStrOpt = obj.at("mesh").as_string();
	if (!meshStrOpt)
	{
		NE_WARN("Not a valid string format for mesh!");
		return;
	}
	const auto& meshName = meshStrOpt.value();

	// try to find mesh in scene map
	const Scene::TValueUMap& meshMap = sceneMap.at(SceneNode::Mesh);
	if (meshMap.find(meshName) == meshMap.end())
	{
		NE_WARN("Did not find mesh with name: {}... skipping.", meshName);
		return;
	}

	const auto& meshObjOpt = meshMap.at(meshName).as_object();
	if (!meshObjOpt)
	{
		NE_WARN("Not a valid map format for mesh: {}... skipping.", meshName);
		return;
	}
	const auto& meshObjMap = meshObjOpt.value();

	// yea im lazy, gonna wrap everything in one try catch instead...
	try {
		// also deserializes material
		Mesh::Deserialize(newEntity, meshObjMap, sceneMap);
	}
	catch (std::exception& e) {
		NE_ERROR("Failed to make renderable mesh and material. with error: {}", e.what());
		return;
	}
}

static void MakeCamera(Entity* newEntity, const Scene::TValueMap& obj, const Scene::TSceneMap& sceneMap)
{
	if (obj.find("camera") == obj.end())
		return;

	const auto& cameraStrOpt = obj.at("camera").as_string();
	if (!cameraStrOpt)
	{
		NE_WARN("Not a valid string format for camera name!");
		return;
	}
	const auto& cameraName = cameraStrOpt.value();

	// try to find camera in scene map
	const Scene::TValueUMap& cameraMap = sceneMap.at(SceneNode::Camera);
	if (cameraMap.find(cameraName) == cameraMap.end())
	{
		NE_WARN("Did not find camera with name: {}... skipping.", cameraName);
		return;
	}

	const auto& cameraObjOpt = cameraMap.at(cameraName).as_object();
	if (!cameraObjOpt)
	{
		NE_WARN("Not a valid map format for camera: {}... skipping.", cameraName);
		return;
	}
	const auto& cameraObjMap = cameraObjOpt.value();

	// case on perspective/orthographic
	if (cameraObjMap.find("perspective") != cameraObjMap.end())
	{
		const auto& perspectiveObjOpt = cameraObjMap.at("perspective").as_object();
		if (!perspectiveObjOpt)
		{
			NE_WARN("Not a valid map format for camera: {}... skipping.", cameraName);
			return;
		}
		const auto& perspectiveObj = perspectiveObjOpt.value();

		auto get = [&perspectiveObj](const char* key) { return (float)perspectiveObj.at(key).as_float(); };
		float aspect = get("aspect");
		float fov = get("vfov");
		float near = get("near");
		float far = get("far");

		newEntity->AddComponent<CameraComponent>(Camera::Scene, false, near, far, fov, aspect);
	}
	else
	{
		// TODO: handle camera orthographic
		NE_WARN("Orthographic cameras are not yet supported");
	}
}

static Entity* MakeNode(Scene* scene, Scene::TSceneMap& sceneMap, const std::string& nodeName, Entity* parent)
{
	NE_DEBUG("Creating entity", Logger::CYAN, Logger::BOLD);

	const Scene::TValueUMap& nodeMap = sceneMap[SceneNode::Node];

	if (nodeMap.find(nodeName) == nodeMap.end())
	{
		std::cout << "Did not find this node with name: " << nodeName << std::endl;
		return nullptr;
	}
	const auto& value = nodeMap.at(nodeName);

	Entity* newEntity = nullptr;

	// make transform
	glm::vec3 p, s;
	glm::quat r;
	if (LoadAsTransform(value, p, r, s))
	{
		if (!parent)
			newEntity = scene->Instantiate(nodeName, p, r, s);
		else {
			newEntity = parent->AddChild(nodeName, p, r, s);
		}

		assert(newEntity && "Entity should be non-null");
	}

	const auto& obj = value.as_object().value();

	MakeMesh(newEntity, obj, sceneMap);
	MakeCamera(newEntity, obj, sceneMap);

	// recursively call on children
	{
		if (obj.find("children") == obj.end())
			return newEntity; // no children!

		const auto& childrenArrOpt = obj.at("children").as_array();
		if (!childrenArrOpt)
		{
			std::cout << "Not a valid array format for children!\n";
			return newEntity; // not valid children array
		}

		NE_INFO("Has {} children", childrenArrOpt.value().size());
		for (const auto& childValue : childrenArrOpt.value())
		{
			const auto& childOpt = childValue.as_string();
			if (!childOpt)
			{
				std::cout << "Not a valid string format for child!\n";
				continue;
			}

			// At last, the recursive call!
			MakeNode(scene, sceneMap, childOpt.value(), newEntity);
		}
	}

	return newEntity;
}

void Scene::Deserialize(const std::string& path)
{
	NE_INFO("Loading scene: {}", path);

	TSceneMap sceneMap; // entire scene map

	std::vector<std::string> roots;
	bool foundScene = false;

	try {
		sejp::value loaded = sejp::load(Files::Path(path));
		auto& arr = loaded.as_array().value();

		// do a first parse of the file, put everything into sceneMap, hashed by name
		for (const auto& elem : arr)
		{
			const auto& obj = elem.as_object();
			if (!obj)
				continue;

			const auto& objMap = obj.value();

			// define some lambdas for parsing...
			auto GET_STR = [&objMap](const char* key) { return objMap.at(key).as_string().value(); };

			try {
				SceneNode::Type type = SceneNode::ObjectType(GET_STR("type"));

				// insert into scene map
				if (sceneMap.find(type) == sceneMap.end())
					sceneMap[type] = {};
				sceneMap[type].insert({ GET_STR("name"), elem });

				// add all node names to be used later
				if (type == SceneNode::Scene)
				{
					if (foundScene)
						throw std::runtime_error("More than one scene object found!");

					foundScene = true;
					const auto& rootsArr = objMap.at("roots").as_array().value();
					roots.resize(rootsArr.size());

					for (const auto& [id, root] : Enumerate(rootsArr))
					{
						roots[id] = root.as_string().value();
					}
				}
			}
			catch (std::exception& e) {
				NE_WARN("Skipping this object due to an error: {}", e.what());
				continue;
			}
		}

		if (sceneMap.find(SceneNode::Node) == sceneMap.end()) 
			NE_WARN("Scene is empty");
		else 
		{
			// traverse all node objects
			for (const auto& root : roots)
			{
				MakeNode(this, sceneMap, root, nullptr);
			}
		}

		NE_INFO("Finished loading scene");
	}
	catch (std::exception& e) {
		NE_ERROR("Failed to deserialize a scene with path: [{}]. Error: {}", std::move(path), e.what());
	}
}

void Scene::PushObjectInstances(ObjectInstance&& instance)
{
	m_ObjectInstances.emplace_back(std::move(instance));
}

// rendering will be on debug cam, unless it is in scene mode
CameraComponent* Scene::GetRenderCam() const
{
	if (SceneManager::Get()->getCameraMode() == CameraMode::Scene)
		return sceneCam();
	else
		return debugCam();
}

// culling will be on scene cam, unless it is in user mode
inline CameraComponent* Scene::GetCullCam() const
{
	if (SceneManager::Get()->getCameraMode() == CameraMode::User)
		return debugCam();
	else
		return sceneCam();
}

CameraComponent* Scene::sceneCam() const
{
	if (m_SceneCameras.empty())
		return nullptr;

	CameraComponent* leastCam = nullptr;
	int leastPrio = 10000;
	for (CameraComponent* cam : m_SceneCameras)
	{
		if (cam->priority < leastPrio) {
			leastCam = cam;
			leastPrio = cam->priority;
		}
	}

	return leastCam;
}

inline CameraComponent* Scene::debugCam() const
{
	return m_DebugCamera;
}

void Scene::UpdateWorldUniform()
{
	m_SceneInfo.SKY_DIRECTION.x = 0.0f;
	m_SceneInfo.SKY_DIRECTION.y = 0.0f;
	m_SceneInfo.SKY_DIRECTION.z = 1.0f;

	m_SceneInfo.SKY_ENERGY.r = 0.1f;
	m_SceneInfo.SKY_ENERGY.g = 0.1f;
	m_SceneInfo.SKY_ENERGY.b = 0.2f;

	m_SceneInfo.SUN_DIRECTION.x = 6.0f / 23.0f;
	m_SceneInfo.SUN_DIRECTION.y = 13.0f / 23.0f;
	m_SceneInfo.SUN_DIRECTION.z = 18.0f / 23.0f;

	m_SceneInfo.SUN_ENERGY.r = 1.0f;
	m_SceneInfo.SUN_ENERGY.g = 1.0f;
	m_SceneInfo.SUN_ENERGY.b = 0.9f;
}