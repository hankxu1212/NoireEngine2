#pragma once

#include <memory>
#include <vector>
#include <list>
#include <utils/UUID.hpp>
#include <string>

#include "Transform.hpp"

class Entity
{
public:
	template<typename... TArgs>
	void AddChild(const TArgs&... args)
	{
		m_Children.emplace_back(std::make_unique<Entity>(args...));
		m_Children.back()->m_Parent = this;
	}

public:
	std::unique_ptr<Transform>				s_Transform;

private:
	Entity*									m_Parent = nullptr;
	std::list<std::unique_ptr<Entity>>		m_Children; // manages its children

	UUID id;
	std::string name;
};

