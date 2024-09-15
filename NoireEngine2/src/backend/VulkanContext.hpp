#pragma once

#include "core/resources/Module.hpp"

class VulkanContext : public Module::Registrar<VulkanContext>
	inline static const bool Registered = RegisterCreate(Stage::Render, DestroyStage::Post, Requires<Window>());
{
};

