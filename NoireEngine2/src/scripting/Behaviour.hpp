#pragma once

#include "renderer/components/Component.hpp"
#include "core/events/Event.hpp"
#include "utils/Logger.hpp"
#include "core/Time.hpp"

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

	virtual const char* getName() override { return "Behaviour"; }
	virtual const char* getClassName() const { return NE_NULL_STR; }

	bool enabled;
};