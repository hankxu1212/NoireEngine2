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

    Keyframe() : 
        timestamp(0.0f), position(glm::vec3(0.0f)), rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), scale(glm::vec3(1.0f)){
    }

    Keyframe(float ts, const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scl) : 
        timestamp(ts), position(pos), rotation(rot), scale(scl) {
    }

    Keyframe(float ts, const glm::vec3& pos) :
        timestamp(ts), position(pos), rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), scale(glm::vec3(1.0f)) {
    }

    Keyframe(float ts, const glm::quat& rot) :
        timestamp(ts), position(glm::vec3(0.0f)), rotation(rot), scale(glm::vec3(1.0f)) {
    }

    static glm::vec3 InterpolatePosition(float t, const Keyframe& kf1, const Keyframe& kf2);
    static glm::quat InterpolateRotation(float t, const Keyframe& kf1, const Keyframe& kf2);
    static glm::vec3 InterpolateScale(float t, const Keyframe& kf1, const Keyframe& kf2);
};


