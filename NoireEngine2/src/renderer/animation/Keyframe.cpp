#include "Keyframe.hpp"
#include "Animation.hpp"

glm::vec3 Keyframe::InterpolatePosition(float t, const Keyframe& kf1, const Keyframe& kf2)
{
    float t1 = kf1.timestamp;
    float t2 = kf2.timestamp;
    return kf1.position + (kf2.position - kf1.position) * ((t - t1) / (t2 - t1));
}

glm::quat Keyframe::InterpolateRotation(float t, const Keyframe& kf1, const Keyframe& kf2)
{
    float t1 = kf1.timestamp;
    float t2 = kf2.timestamp;
    return glm::mix(kf1.rotation, kf2.rotation, (t - t1) / (t2 - t1));
}

glm::vec3 Keyframe::InterpolateScale(float t, const Keyframe& kf1, const Keyframe& kf2)
{
    float t1 = kf1.timestamp;
    float t2 = kf2.timestamp;
    return kf1.scale + (kf2.scale - kf1.scale) * ((t - t1) / (t2 - t1));
}