#include "Material.hpp"
#include "backend/pipeline/ObjectPipeline.hpp"
#include "glm/gtx/string_cast.hpp"
#include "utils/Logger.hpp"

#include "LambertianMaterial.hpp"
#include "EnvironmentMaterial.hpp"
#include "MirrorMaterial.hpp"
#include "PBRMaterial.hpp"

Material* Material::Deserialize(const Scene::TValueMap& obj)
{
	auto lambertianIt = obj.find("lambertian");
	if (lambertianIt != obj.end())
		return LambertianMaterial::Deserialize(obj);
	
	auto pbrIt = obj.find("pbr");
	if (pbrIt != obj.end())
		return PBRMaterial::Deserialize(obj);

	auto mirrorIt = obj.find("mirror");
	if (mirrorIt != obj.end())
		return MirrorMaterial::Deserialize(obj);

	auto environmentIt = obj.find("environment");
	if (environmentIt != obj.end())
		return EnvironmentMaterial::Deserialize(obj);

	return nullptr;
}

std::shared_ptr<Material> Material::CreateDefault()
{
	return LambertianMaterial::Create();
}