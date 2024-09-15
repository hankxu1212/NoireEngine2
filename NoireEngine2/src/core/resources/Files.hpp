#pragma once

#include "core/resources/Module.hpp"

#include <filesystem>

class Files : public Module::Registrar<Files>
{
	inline static const bool Registered = Register(UpdateStage::Never, DestroyStage::Normal);
public:
	static std::optional<std::string> Read(const std::filesystem::path& path);
};

