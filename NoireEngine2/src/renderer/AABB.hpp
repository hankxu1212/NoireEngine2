#pragma once

#include "glm/glm.hpp"
#include "Frustum.hpp"

struct AABB 
{
    AABB() = default;

    glm::vec3 min, max, originMin, originMax;

    void Update(const glm::mat4& model);
};