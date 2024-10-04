#include "Material.hpp"
#include "backend/pipeline/ObjectPipeline.hpp"
#include "glm/gtx/string_cast.hpp"
#include "utils/Logger.hpp"
#include "LambertianMaterial.hpp"

Material* Material::Deserialize(const Scene::TValueMap& obj)
{
	// TODO: add conditioning
	return LambertianMaterial::Deserialize(obj);
}

std::shared_ptr<Material> Material::CreateDefault()
{
	return LambertianMaterial::Create();
}