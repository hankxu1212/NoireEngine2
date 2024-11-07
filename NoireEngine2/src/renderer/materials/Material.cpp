#include "Material.hpp"
#include "renderer/Renderer.hpp"
#include "glm/gtx/string_cast.hpp"
#include "utils/Logger.hpp"

#include "LambertianMaterial.hpp"
#include "PBRMaterial.hpp"

Material* Material::Deserialize(const Scene::TValueMap& obj)
{
	auto lambertianIt = obj.find("lambertian");
	if (lambertianIt != obj.end())
		return LambertianMaterial::Deserialize(obj);
	
	auto pbrIt = obj.find("pbr");
	if (pbrIt != obj.end())
		return PBRMaterial::Deserialize(obj);

	return CreateDefault().get();
}

std::shared_ptr<Material> Material::CreateDefault()
{
	return PBRMaterial::Create();
}