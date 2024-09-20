#include "Scene.hpp"

#include "renderer/Camera.hpp"
#include "renderer/components/Components.hpp"
#include "core/Time.hpp"
#include "Entity.hpp"
#include "utils/sejp/sejp.hpp"
#include "core/resources/Files.hpp"
#include "SceneNode.hpp"
#include "utils/Enumerate.hpp"

#include <iostream>
#include <unordered_map>
#include <unordered_set>

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

/*
static Transform* LoadAsTransform(sejp::value& val)
{
	try {
		const auto& obj = val.as_object().value();

		const auto& translation = obj.at("translation").as_array().value();
		assert(translation.size() == 3);

		const auto& rotation = obj.at("rotation").as_array().value();
		assert(rotation.size() == 4);

		const auto& scale = obj.at("scale").as_array().value();
		assert(scale.size() == 3);

		glm::vec3 pos{ translation[0].as_float(), translation[1].as_float(), translation[2].as_float() };

		glm::quat rot = glm::quat(rotation[0].as_float(), rotation[1].as_float(), rotation[2].as_float(), rotation[3].as_float());

		glm::vec3 scl{ scale[0].as_float(), scale[1].as_float(), scale[2].as_float() };

		return new Transform(pos, rot, scl);
	}
	catch (std::exception& e) {
		std::cout << "Failed to deserialize value as Transform: " << e.what() << std::endl;
		return nullptr;
	}
}
*/

void Scene::Deserialize(const std::string& path)
{
	std::cout << "Loading scene: >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << path << std::endl;

	typedef std::unordered_map<std::string, sejp::value> value_map_t; // { name, object map }
	std::unordered_map<SceneNode::Type, value_map_t> sceneMap; // entire scene map

	std::vector<std::string> roots = {};

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
					static bool foundScene = false;

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
				std::cout << "Skipping this object due to an error: " << e.what() << std::endl;
				continue;
			}
		}


		if (sceneMap.find(SceneNode::Node) == sceneMap.end()) 
		{
			std::cout << "Warning: Scene is empty\n";
		}
		else 
		{
			// now we will start building the scene graph
			// we start by traversing NODE objects first
			value_map_t& nodeMap = sceneMap[SceneNode::Node];

			// this function will be called recursively
			auto MAKE_SINGLE_NODE = [&](const std::string& nodeName) {
				if (nodeMap.find(nodeName) == nodeMap.end())
				{
					std::cout << "Did not find this node with name: " << nodeName << std::endl;
					return;
				}

				//Transform *t = LoadAsTransform(nodeMap[nodeName]);
				//Instantiate();
			};

			for (const auto& root : roots)
			{
				MAKE_SINGLE_NODE(root);
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

Entity* Scene::Instantiate(Transform* t)
{
	return Instantiate(t->position(), t->rotation(), t->scale());
}

void Scene::PushObjectInstances(const ObjectInstance&& instance)
{
	m_ObjectInstances.push_back(instance);
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