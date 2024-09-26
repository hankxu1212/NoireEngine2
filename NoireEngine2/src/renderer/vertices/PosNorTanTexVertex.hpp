#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/hash.hpp"

struct PosNorTanTexVertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec4 tangent;
	glm::vec2 texCoord;

	bool operator==(const PosNorTanTexVertex& other) const {
		return position == other.position &&
			normal == other.normal &&
            tangent == other.tangent &&
			texCoord == other.texCoord;
	}
};

static_assert(sizeof(PosNorTanTexVertex) == 12 + 12 + 16 + 8);

namespace std {
    template<>
    struct hash<PosNorTanTexVertex> {
        std::size_t operator()(const PosNorTanTexVertex& vertex) const {
            std::size_t h1 = std::hash<glm::vec3>()(vertex.position);
            std::size_t h2 = std::hash<glm::vec3>()(vertex.normal);
            std::size_t h3 = std::hash<glm::vec4>()(vertex.tangent);
            std::size_t h4 = std::hash<glm::vec2>()(vertex.texCoord);

            // Combine the individual hashes using XOR and bit shifting
            return (((h1 ^ (h2 << 1)) >> 1) ^ (h3 << 1)) ^ (h4 << 1);
        }
    };
}