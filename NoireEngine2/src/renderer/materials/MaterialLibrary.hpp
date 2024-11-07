#pragma once

#include "core/resources/Module.hpp"
#include "backend/images/Image2D.hpp"

class MaterialLibrary : public Module::Registrar<MaterialLibrary>
{
	inline static const bool Registered = Register(UpdateStage::Never, DestroyStage::Normal);
public:
	MaterialLibrary();

	~MaterialLibrary();

	uint32_t Add(std::shared_ptr<Image2D>&);

	void Remove(std::shared_ptr<Image2D>);

	uint32_t Find();

	void Destroy();

	const std::vector<std::shared_ptr<Image2D>>& GetTextures() const { return textures; }

private:
	std::vector<std::shared_ptr<Image2D>> textures;
};

