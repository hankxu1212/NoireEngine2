#pragma once

#include "Material.hpp"

class LambertianMaterial : public Material
{
public:
	struct CreateInfo
	{
		std::string name;
		glm::vec3 albedo;
		std::string texturePath;

		CreateInfo() = default;

		CreateInfo(const std::string& n, const glm::vec3& a, const std::string& tPath) :
			name(n), albedo(a), texturePath(tPath) {
		}

		CreateInfo(const CreateInfo& other) :
			name(other.name), albedo(other.albedo), texturePath(other.texturePath) {
		}
	};
};
