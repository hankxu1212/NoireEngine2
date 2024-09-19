#pragma once

#include "renderer/scene/Entity.hpp"
#include "core/events/Event.hpp"
#include "ScriptingEngine.hpp"

/**
 * A script attached to in game entities (Monobehavior basically)
 * Should use the Create() function to create scripts so they get ported into script manager!
 */
class Behaviour : public Component
{
public:
	virtual void Awake() {}
	virtual void Start() {}
	virtual void Update() {};
	virtual void Shutdown() {}
	virtual void HandleEvent(Event& event) {}

	virtual void Inspect() {}
	virtual void Debug() {}

	virtual const char* getClassName() const { return ""; }
};