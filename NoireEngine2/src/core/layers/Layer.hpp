#pragma once

#include <string>

#include "core/Core.hpp"
#include "core/events/Event.hpp"

/**
  * A Noire Engine application layer. Can be attached and detached on runtime.
  * Destruction/Detachment order between layers are not reinforced and runs in the order 
  * they were initially pushed into the stack
*/
class Layer
{
public:
	Layer(const std::string& _name = "DefaultLayer") : name(_name) {}
	virtual ~Layer() = default; // correspond to between Normal and Post destruction stage

	virtual void OnAttach() {}
	virtual void OnDetach() {} // corresponds to between Pre and Normal destruction stage
	virtual void OnUpdate() {}
	virtual void OnImGuiRender() {}
	virtual void OnViewportRender() {}
	virtual void OnEvent(Event& event) {}
public:
	std::string name;
};
