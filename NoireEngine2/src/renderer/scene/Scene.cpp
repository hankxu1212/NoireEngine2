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

#include "core/Timer.hpp"

Scene::Scene(const std::string& path)
{
	if (!Files::Exists(Files::Path(path)))
		NE_ERROR("Path " + path + " does not exist.");

	Deserialize(path);
	InstantiateCoreScripts();

	m_ObjectInstances.resize(MAX_WORKFLOWS);
}

Scene::~Scene()
{
	Unload();
}

void Scene::Unload()
{
}

static bool sceneCamDirty = true; // prompt update again

void Scene::Update()
{
	sceneCamDirty = true;

	// update entities and components
	Entity::root().Update();

	UpdateSceneInfo();
}

void Scene::Render()
{
	if (GetRenderCam() == nullptr)
	{
		NE_ERROR("Render camera is null");
		return;
	}

	for (auto& instances : m_ObjectInstances)
		instances.clear();

	m_GizmosInstances.clear();
	m_MatrixStack.Clear();

	// push all transforms to pipeline
	for (auto& child : Entity::root().children()) 
	{
		child->RenderPass(m_MatrixStack);
 	}
}

static std::unordered_map<std::string, Entity*> nameToEntityMap;

static void LoadTransform(const Scene::TValueMap& obj, glm::vec3& outPosition, glm::quat& outRotation, glm::vec3& outScale)
{
	auto translationIt = obj.find("translation");
	if (translationIt != obj.end())
	{
		try {
			outPosition = translationIt->second.as_vec3();
		}
		catch (std::exception& e) {
			NE_WARN("Translation is corrupted {}", e.what());
		}
	}

	auto rotationIt = obj.find("rotation");
	if (rotationIt != obj.end())
	{
		try {
			auto q = rotationIt->second.as_vec4();
			outRotation = glm::quat(q[3], q[0], q[1], q[2]);
		}
		catch (std::exception& e) {
			NE_WARN("Rotation is corrupted {}", e.what());
		}
	}

	auto scaleIt = obj.find("scale");
	if (scaleIt != obj.end())
	{
		try {
			outScale = scaleIt->second.as_vec3();
		}
		catch (std::exception& e) {
			NE_WARN("Scale is corrupted {}", e.what());
		}
	}
}

static Scene::TFoundInfo Find(const char* key, SceneNode::Type type, const Scene::TValueMap& obj, const Scene::TSceneMap& sceneMap)
{
	static Scene::TFoundInfo NONE = { std::nullopt, "" };

	auto it = obj.find(key);
	if (it == obj.end())
		return NONE;

	const auto& strOpt = it->second.as_string();
	if (!strOpt)
	{
		NE_WARN("Not a valid string format for {}!", key);
		return NONE;
	}
	const auto& name = strOpt.value();

	const Scene::TValueUMap& map = sceneMap.at(type);
	auto objIt = map.find(name);
	if (objIt == map.end())
	{
		NE_WARN("Did not find {} with name: {}... skipping.", key, name);
		return NONE;
	}

	return std::make_pair(objIt->second.as_object(), name);
}

static void MakeMesh(Entity* newEntity, const Scene::TValueMap& obj, const Scene::TSceneMap& sceneMap)
{
	const auto& opt = Find("mesh", SceneNode::Mesh, obj, sceneMap);
	if (!opt.first)
		return;
	const auto& dict = opt.first.value();

	// deserialize mesh and material here
	try {
		Mesh::Deserialize(newEntity, dict, sceneMap);
	}
	catch (std::exception& e) {
		NE_ERROR("Failed to make renderable mesh and material. with error: {}", e.what());
		return;
	}
}

static void MakeCamera(Entity* newEntity, const Scene::TValueMap& obj, const Scene::TSceneMap& sceneMap)
{
	const auto& opt = Find("camera", SceneNode::Camera, obj, sceneMap);
	if (!opt.first)
		return;
	const auto& dict = opt.first.value();

	// case on perspective/orthographic
	if (dict.find("perspective") != dict.end())
	{
		const auto& perspectiveObjOpt = dict.at("perspective").as_object();
		if (!perspectiveObjOpt)
		{
			NE_WARN("Not a valid map format for camera: {}... skipping.", opt.second);
			return;
		}
		const auto& perspectiveObj = perspectiveObjOpt.value();

		auto get = [&perspectiveObj](const char* key) { return (float)perspectiveObj.at(key).as_float(); };
		float aspect = get("aspect");
		float fov = get("vfov");
		float near = get("near");
		float far = get("far");

		newEntity->AddComponent<CameraComponent>(Camera::Scene, false, near, far, fov, aspect);

		int priority = 0;
		if (Application::GetSpecification().CameraName == opt.second)
			priority = -1000;

		newEntity->GetComponent<CameraComponent>()->priority = priority;
	}
	else
	{
		// TODO: handle camera orthographic
		NE_WARN("Orthographic cameras are not yet supported");
	}
}

static void MakeLight(Entity* newEntity, const Scene::TValueMap& obj, const Scene::TSceneMap& sceneMap)
{
	const auto& opt = Find("light", SceneNode::Light, obj, sceneMap);
	if (!opt.first)
		return;
	const auto& dict = opt.first.value();

	const auto& tintArr = dict.at("tint").as_array().value();
	Color3 color(tintArr[0].as_float(), tintArr[1].as_float(), tintArr[2].as_float());

	if (dict.find("sun") != dict.end())
	{
		const auto& lightObj = dict.at("sun").as_object().value();
		float strength = lightObj.at("strength").as_float();
		newEntity->AddComponent<Light>(Light::Type::Directional, color, strength);
		
		float angle = lightObj.at("angle").as_float();
		newEntity->transform()->SetRotation(glm::quat(glm::vec3(std::sin(angle), std::cos(angle), 0)));
	}
	else if (dict.find("sphere") != dict.end())
	{
		const auto& lightObj = dict.at("sphere").as_object().value();
		float radius = lightObj.at("radius").as_float();
		float power = lightObj.at("power").as_float();

		if (lightObj.find("limit") == lightObj.end())
			newEntity->AddComponent<Light>(Light::Type::Point, color, power, radius);
		else {
			float limit = lightObj.at("limit").as_float();
			newEntity->AddComponent<Light>(Light::Type::Point, color, power, radius, limit);
		}
	}
	else if (dict.find("spot") != dict.end())
	{
		const auto& lightObj = dict.at("spot").as_object().value();

		float power = lightObj.at("power").as_float();
		float fov = lightObj.at("fov").as_float();
		float radius = lightObj.at("radius").as_float();
		float blend = lightObj.at("blend").as_float();

		if (lightObj.find("limit") == lightObj.end())
			newEntity->AddComponent<Light>(Light::Type::Spot, color, power, radius, fov, blend);
		else {
			float limit = lightObj.at("limit").as_float();
			newEntity->AddComponent<Light>(Light::Type::Spot, color, power, radius, fov, blend, limit);
		}
	}
}

static void MakeAnimation(const Scene::TSceneMap& sceneMap)
{
	if (sceneMap.find(SceneNode::Driver) == sceneMap.end())
		return; // no animation objects

	for (const auto& nameValuePair : sceneMap.at(SceneNode::Driver))
	{
		const auto& driverObjOpt = nameValuePair.second.as_object();
		if (!driverObjOpt) {
			NE_WARN("Could not read driver object as map.");
			continue;
		}

		const auto& nodeName = driverObjOpt.value().at("node").as_string().value();
		if (nameToEntityMap.find(nodeName) == nameToEntityMap.end()) {
			NE_WARN("No entity named" + nodeName + "exists.");
			continue;
		}

		const auto& animName = driverObjOpt.value().at("name").as_string().value();

		std::shared_ptr<Animation> anim = Animation::Create(animName, animName);
		anim->Load(driverObjOpt.value());

		Entity* entity = nameToEntityMap.at(nodeName);
		entity->AddComponent<Animator>(anim);
	}
}

static void MakeEnvironment(Scene* scene, const Scene::TSceneMap& sceneMap)
{
	auto it = sceneMap.find(SceneNode::Environment);
	if (it == sceneMap.end())
		return;

	int i = 0;
	for (const auto& nameValuePair : it->second)
	{
		i++;
		if (i > 1)
			NE_ERROR("Multiple environment objects found.. Aborting!");

		const auto& environmentObjOpt = nameValuePair.second.as_object();
		if (!environmentObjOpt) {
			NE_WARN("Could not read driver object as map.");
			continue;
		}

		const auto& name = environmentObjOpt.value().at("radiance").as_texPath().value();
		scene->AddSkybox("../scenes/examples/" + name);
	}
}

static Entity* MakeNode(Scene* scene, Scene::TSceneMap& sceneMap, const std::string& nodeName, Entity* parent)
{
	NE_DEBUG("Creating entity", Logger::CYAN, Logger::BOLD);

	const Scene::TValueUMap& nodeMap = sceneMap[SceneNode::Node];

	auto nodeIt = nodeMap.find(nodeName);
	if (nodeIt == nodeMap.end())
	{
		NE_WARN("Did not find this node with name: {}", nodeName);
		return nullptr;
	}
	const auto& obj = nodeIt->second.as_object().value();

	Entity* newEntity = nullptr;

	// make transform
	glm::vec3 p(0), s(1);
	glm::quat r(1,0,0,0);
	LoadTransform(obj, p, r, s);
	if (!parent)
		newEntity = scene->Instantiate(nodeName, p, r, s);
	else
		newEntity = parent->AddChild(nodeName, p, r, s);

	nameToEntityMap[nodeName] = newEntity;

	MakeMesh(newEntity, obj, sceneMap);
	MakeCamera(newEntity, obj, sceneMap);
	MakeLight(newEntity, obj, sceneMap);

	// recursively call on children
	{
		auto childrenIt = obj.find("children");
		if (childrenIt == obj.end())
			return newEntity; // no children!

		const auto& childrenArrOpt = childrenIt->second.as_array();
		if (!childrenArrOpt)
		{
			NE_WARN("Not a valid array format for children!");
			return newEntity; // not valid children array
		}

		NE_INFO("Has {} children", childrenArrOpt.value().size());
		for (const auto& childValue : childrenArrOpt.value())
		{
			const auto& childOpt = childValue.as_string();
			if (!childOpt)
			{
				NE_WARN("Not a valid string format for child!");
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

			MakeAnimation(sceneMap);
			MakeEnvironment(this, sceneMap);
		}

		NE_INFO("Finished loading scene");
	}
	catch (std::exception& e) {
		NE_ERROR("Failed to deserialize a scene with path: [{}]. Error: {}", std::move(path), e.what());
	}
}

void Scene::AddSkybox(const std::string& path, SkyboxType type)
{
	switch (type)
	{
	case SkyboxType::HDR:
		m_Skybox = ImageCube::Create(Files::Path(path), true);
		break;
	case SkyboxType::RGB:
		m_Skybox = ImageCube::Create(Files::Path(path), false);
		break;
	}
}

void Scene::PushObjectInstance(ObjectInstance&& instance, uint32_t index)
{
	m_ObjectInstances[index].emplace_back(std::move(instance));
}

void Scene::PushGizmosInstance(GizmosInstance* instance)
{
	m_GizmosInstances.emplace_back(instance);
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
CameraComponent* Scene::GetCullingCam() const
{
	if (SceneManager::Get()->getCameraMode() == CameraMode::User)
		return debugCam();
	else
		return sceneCam();
}

CameraComponent* Scene::sceneCam() const
{
	static CameraComponent* SceneCamera = nullptr;

	if (!sceneCamDirty)
		return SceneCamera;

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

	SceneCamera = leastCam;
	sceneCamDirty = false;

	return SceneCamera;
}

inline CameraComponent* Scene::debugCam() const
{
	return m_DebugCamera;
}

void Scene::InstantiateCoreScripts()
{
	Entity* eCam = Instantiate("Core::Debug Camera", glm::vec3(0, 0, 10), glm::quat(1, 0, 0, 0));
	eCam->AddComponent<Core::SceneNavigationCamera>();
	eCam->AddComponent<CameraComponent>(Camera::Type::Debug);

	// setup debug camera and camera script
	m_DebugCamera = eCam->GetComponent<CameraComponent>();
	m_UserNavigationCamera = eCam->GetComponent<Core::SceneNavigationCamera>();

	Entity* eInput = Instantiate("Core::Input");
	eInput->AddComponent<Core::Input>();

	// if no camera found, add a rendering camera
	if (m_SceneCameras.empty()) {
		Entity* autoCam = Instantiate("Auto-Instantiated Rendering Camera", glm::vec3(0, 0, 10), glm::quat(1, 0, 0, 0));
		autoCam->AddComponent<CameraComponent>();
	}
}

void Scene::UpdateSceneInfo()
{
	int i = 0;
	for (Light* light : m_SceneLights)
	{
		memcpy(&m_SceneInfo.lights[i], &light->GetLightUniform(), sizeof(LightUniform));
		i++;
	}
	m_SceneInfo.numLights = i;

	const glm::vec3& pos = GetRenderCam()->GetTransform()->position();
	m_SceneInfo.cameraPosition = {pos.x, pos.y, pos.z, 0};
}
