#include "MaterialLibrary.hpp"
#include "core/resources/Files.hpp"
#include "utils/Logger.hpp"

MaterialLibrary::MaterialLibrary()
{
}

MaterialLibrary::~MaterialLibrary()
{
	Destroy();
}

uint32_t MaterialLibrary::Add(std::shared_ptr<Image2D>& newTex)
{
	textures.emplace_back(newTex);
	return static_cast<uint32_t>(textures.size() - 1);
}

void MaterialLibrary::Remove(std::shared_ptr<Image2D>)
{
	NE_WARN("Removing a texture from material library is not yet supported in NE2");
}

uint32_t MaterialLibrary::Find()
{
	NE_WARN("Finding a texture from material library is not yet supported in NE2");
	return 0;
}

void MaterialLibrary::Destroy()
{
	textures.clear();
}
