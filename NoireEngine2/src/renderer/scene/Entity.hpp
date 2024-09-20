#pragma once

#include <memory>
#include <vector>
#include <list>
#include <set>
#include <utils/UUID.hpp>
#include <string>

#include "Transform.hpp"
#include "renderer/components/Components.hpp"
#include "Scene.hpp"

#include "renderer/components/renderer_components/RendererComponent.hpp"

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

	template<typename... TArgs>
	Entity(Scene* scene, TArgs&... args) :
		m_Scene(scene), s_Transform(std::make_unique<Transform>(args...)) {
	}

	template<typename... TArgs>
	Entity(Scene* scene, const char* name, TArgs&... args) :
		m_Scene(scene), m_Name(name), s_Transform(std::make_unique<Transform>(args...)) {
	}

	~Entity();

public:
	/**
	 * Add a child to an entity.
	 * 
	 * \param ...args parameters to construct the new entity
	 * \return a pointer to the newly created entity
	 */
	template<typename... TArgs>
	Entity* AddChild(TArgs&... args)
	{
		m_Children.emplace_back(std::make_unique<Entity>(m_Scene, args...));
		m_Children.back()->m_Parent = this;
		return m_Children.back().get();
	}

	// Variation of AddChild where it takes in a scene to overwrite current scene.
	template<typename... TArgs>
	Entity* AddChild(Scene* scene, TArgs&... args)
	{
		m_Scene = scene;
		m_Children.emplace_back(std::make_unique<Entity>(scene, args...));
		m_Children.back()->m_Parent = this;
		return m_Children.back().get();
	}

	// Adds a component to an entity
	template<typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		m_Components.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
		
		m_Components.back()->SetEntity(this);

		T& newComponent = *dynamic_cast<T*>(m_Components.back().get());
		
		m_Scene->OnComponentAdded(*this, newComponent);

		return newComponent;
	}

	template<typename T>
	T* GetComponent() const
	{
		for (const auto& component : m_Components)
		{
			T* c = (T*)component.get();
			if (c == nullptr)
				continue;
			return c;
		}
		return nullptr;
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
	Scene* scene() { return m_Scene; }

	void SetRenderer(RendererComponent* newRenderer) { m_RendererComponent = newRenderer; }
	RendererComponent* renderer() { return m_RendererComponent; }

private:
	std::unique_ptr<Transform>				s_Transform;
	Entity*									m_Parent = nullptr;
	std::list<std::unique_ptr<Entity>>		m_Children; // manages its children
	std::vector<std::unique_ptr<Component>>	m_Components;
	Scene*									m_Scene = nullptr;
	RendererComponent*						m_RendererComponent;

	UUID m_Id;
	std::string m_Name;
};

