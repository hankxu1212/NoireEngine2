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

class Entity
{
public:
	static Entity& root()
	{
		static Entity r;
		return r;
	}

	Entity();
	
	Entity(glm::vec3& position);

	Entity(const char* name);

	Entity(const Entity&);

	~Entity();

public:
	template<typename... TArgs>
	Entity* AddChild(TArgs&... args)
	{
		m_Children.emplace_back(std::make_unique<Entity>(args...));
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

private:
	std::unique_ptr<Transform>				s_Transform;
	Entity*									m_Parent = nullptr;
	std::list<std::unique_ptr<Entity>>		m_Children; // manages its children
	std::vector<Component>					m_Components;

	UUID m_Id;
	std::string m_Name;
};

