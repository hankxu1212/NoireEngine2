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

	struct MaterialPush
	{
		struct { float x, y, z, padding_; } albedo;
		int materialIndex;
	};
	static_assert(sizeof(MaterialPush) == 16 + 4);

	void Push(const CommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout) override;

private:
	glm::vec3						m_Albedo;
	uint32_t						m_AlbedoMapIndex = 0; // index into global texture array
};
