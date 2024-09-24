#include "Scene.hpp"

#include "renderer/Camera.hpp"
#include "renderer/components/Components.hpp"
#include "core/Time.hpp"
#include "Entity.hpp"
#include "core/resources/Files.hpp"
#include "SceneNode.hpp"
#include "utils/Enumerate.hpp"
#include "utils/Logger.hpp"

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <functional>

Scene::Scene()
{
	UpdateWorldUniform();
}

Scene::Scene(const std::string& path)
{
	Deserialize(path);
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
	if (mainCam() == nullptr)
	{
		std::cerr << "Did not find main camera!\n";
		return;
	}

	// TODO: update lightings
	//UpdateWorldUniform();

	// update entities and components
	Entity::root().Update();

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

static Entity* MakeNode(Scene* scene, Scene::TSceneMap& sceneMap, const std::string& nodeName, Entity* parent)
{
	const Scene::TValueUMap& nodeMap = sceneMap[SceneNode::Node];

	if (nodeMap.find(nodeName) == nodeMap.end())
	{
		std::cout << "Did not find this node with name: " << nodeName << std::endl;
		return nullptr;
	}

	const auto& value = nodeMap.at(nodeName);
	Entity* newEntity = nullptr;

	glm::vec3 p, s;
	glm::quat r;
	if (LoadAsTransform(value, p, r, s))
	{
		if (!parent)
			newEntity = scene->Instantiate(p, r, s);
		else
			newEntity = parent->AddChild(p, r, s);
	}

	const auto& obj = value.as_object().value();

	// add mesh/material nodes
	{
		if (obj.find("mesh") == obj.end())
			return nullptr; // no mesh!

		const auto& meshStrOpt = obj.at("mesh").as_string();
		if (!meshStrOpt)
		{
			NE_WARN("Not a valid string format for mesh!");
			goto make_children; // not valid children array
		}
		const auto& meshName = meshStrOpt.value();
		
		// try to find mesh in scene map
		const Scene::TValueUMap& meshMap = sceneMap[SceneNode::Mesh];
		if (meshMap.find(meshName) == meshMap.end())
		{
			NE_WARN("Did not find mesh with name: {}... skipping.", meshName);
			goto make_children; // not valid children array
		}
		
		const auto& meshObjOpt = meshMap.at(meshName).as_object();
		if (!meshObjOpt)
		{
			NE_WARN("Not a valid map format for mesh: {}... skipping.", meshName);
			goto make_children; // not valid children map
		}

		const auto& meshObjMap = meshObjOpt.value();

		// yea im lazy, gonna wrap everything in one try catch instead...
		try {
			// also deserializes material
			Mesh::Deserialize(newEntity, meshObjMap, sceneMap);
		}
		catch (std::exception& e) {
			NE_ERROR("Failed to make renderable mesh and material. with error: {}", e.what());
			goto make_children;
		}
	}

	// add camera nodes
	{

	}

make_children: // recursively call on children
	{
		if (obj.find("children") == obj.end())
			return nullptr; // no children!

		const auto& childrenArrOpt = obj.at("children").as_array();
		if (!childrenArrOpt)
		{
			std::cout << "Not a valid array format for children!\n";
			return nullptr; // not valid children array
		}

		for (const auto& childValue : childrenArrOpt.value())
		{
			const auto& childOpt = childValue.as_string();
			if (!childOpt)
			{
				std::cout << "Not a valid string format for child!\n";
				continue; // not valid children array
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
			auto GET_ARR = [&objMap](const char* key) { return objMap.at(key).as_array().value(); };
			//auto GET_MAP = [&objMap](const char* key) { return objMap.at(key).as_object().value(); };

			try {
				SceneNode::Type type = SceneNode::ObjectType(GET_STR("type"));
				std::string name = GET_STR("name");

				// insert into scene map
				if (sceneMap.find(type) == sceneMap.end())
					sceneMap[type] = {};
				sceneMap[type].insert({ name, elem });

				// add all node names to be used later
				if (type == SceneNode::Scene)
				{
					if (foundScene)
						throw std::runtime_error("More than one scene object found!");

					foundScene = true;
					const auto& rootsArr = GET_ARR("roots");
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
		{
			std::cout << "Warning: Scene is empty\n";
		}
		else 
		{
			// traverse all node objects
			for (const auto& root : roots)
			{
				MakeNode(this, sceneMap, root, nullptr);
			}
		}

		std::cout << "Finished loading scene: >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl;
	}
	catch (std::exception& e) {
		std::cout << "Failed to deserialize a scene at: " << path << "with error: " << e.what() << std::endl;
	}
}

//const std::string name = GET_STR("name");
//const auto& parameters = GET_MAP("perspective");
//auto get = [&parameters](const char* key) { return (float)parameters.at(key).as_float(); };
//float aspect = get("aspect");
//float fov = get("vfov");
//float near = get("near");
//float far = get("far");
//std::cout << aspect << " " << fov << " " << near << " " << far << std::endl;

void Scene::PushObjectInstances(const ObjectInstance&& instance)
{
	m_ObjectInstances.emplace_back(std::move(instance));
}

CameraComponent* Scene::mainCam() const
{
	if (m_Cameras.empty())
		return nullptr;

	return m_Cameras.begin()->second;
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
	m_SceneInfo.SUN_DIRECTION.z = -18.0f / 23.0f;

	m_SceneInfo.SUN_ENERGY.r = 1.0f;
	m_SceneInfo.SUN_ENERGY.g = 0.0f;
	m_SceneInfo.SUN_ENERGY.b = 0.0f;
}