#pragma once

#include <memory>
#include <vector>
#include <list>
#include <set>
#include <utils/UUID.hpp>
#include <string>

#include "Transform.hpp"
#include "Component.hpp"

class TransformMatrixStack;
class Scene;

class Entity
{
public:

	static Entity& root()
	{
		static Entity r;
		return r;
	}

	Entity() = default;

	Entity(Scene* scene);
	
	Entity(Scene* scene, glm::vec3& position);
	Entity(Scene* scene, glm::vec3& position, glm::quat& rotation, glm::vec3& scale);

	Entity(Scene* scene, const char* name);

	Entity(const Entity&);

	~Entity();

public:
	template<typename... TArgs>
	Entity* AddChild(TArgs&... args)
	{
		m_Children.emplace_back(std::make_unique<Entity>(m_Scene, args...));
		m_Children.back()->m_Parent = this;
		return m_Children.back().get();
	}

	template<typename... TArgs>
	Entity* AddChild(Scene* scene, TArgs&... args)
	{
		m_Children.emplace_back(std::make_unique<Entity>(scene, args...));
		m_Children.back()->m_Parent = this;
		return m_Children.back().get();
	}

	// dfs the scene graph and update components
	void Update();

	// stores transforms in a matrix stack, and push transform matrices in a reverse order
	void RenderPass(TransformMatrixStack& matrixStack);

public:
	Entity* parent()				{ return m_Parent; }
	Transform* transform()			{ return s_Transform.get(); }
	UUID& id()						{ return m_Id; }
	std::string& name()				{ return m_Name; }
	std::list<std::unique_ptr<Entity>>& children() { return m_Children; }

	void SetScene(Scene* newScene) { m_Scene = newScene; }

private:
	std::unique_ptr<Transform>				s_Transform;
	Entity*									m_Parent = nullptr;
	std::list<std::unique_ptr<Entity>>		m_Children; // manages its children
	std::vector<Component>					m_Components;
	Scene*									m_Scene = nullptr;

	UUID m_Id;
	std::string m_Name;
};

