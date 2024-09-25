#pragma once

#include <bitset>
#include <optional>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

struct Keyframe
{
    float timestamp;
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    static glm::vec3 InterpolatePosition(float t, const Keyframe& kf1, const Keyframe& kf2);
    static glm::quat InterpolateRotation(float t, const Keyframe& kf1, const Keyframe& kf2);
    static glm::vec3 InterpolateScale(float t, const Keyframe& kf1, const Keyframe& kf2);
};


