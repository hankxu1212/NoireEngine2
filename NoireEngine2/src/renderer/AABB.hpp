#pragma once

#include "glm/glm.hpp"
#include "Frustum.hpp"

class AABB 
{
public:
    AABB() = default;

public:
    glm::vec3 min;
    glm::vec3 max;

    AABB(const glm::vec3& min, const glm::vec3& max) : 
        min(min), max(max) {
    }

    void Update(const glm::mat4& model);
};