#include "Keyframe.hpp"
#include "Animation.hpp"

glm::vec3 Keyframe::InterpolatePosition(float t, const Keyframe& kf1, const Keyframe& kf2)
{
    return kf1.position + (kf2.position - kf1.position) * ((t - kf1.timestamp) / (kf2.timestamp - kf1.timestamp));
}

glm::quat Keyframe::InterpolateRotation(float t, const Keyframe& kf1, const Keyframe& kf2)
{
    return glm::slerp(kf1.rotation, kf2.rotation, (t - kf1.timestamp) / (kf2.timestamp - kf1.timestamp));
}

glm::vec3 Keyframe::InterpolateScale(float t, const Keyframe& kf1, const Keyframe& kf2)
{
    return kf1.scale + (kf2.scale - kf1.scale) * ((t - kf1.timestamp) / (kf2.timestamp - kf1.timestamp));
}