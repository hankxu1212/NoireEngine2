#include "Animator.hpp"
#include "utils/Logger.hpp"
#include "core/Time.hpp"

void Animator::Update()
{
    if (isAnimating)
        Animate();
}

void Animator::Start()
{
    size_t numKeyframes = animation.keyframes.size();
    if (numKeyframes < 2) {
        NE_ERROR("Error: At least two keyframes are required for animation.");
        return;
    }
    isAnimating = true;
}

void Animator::Restart()
{
    currentTime = 0;
    currentKeyframe = 0;
}

void Animator::Stop()
{
    isAnimating = false;
}

void Animator::Inspect()
{
}

void Animator::Animate()
{
    currentTime += Time::DeltaTime;

    if (animation.channels.test((int)Animation::Channel::Position)) {
        GetTransform()->SetPosition(
            Keyframe::InterpolatePosition(currentTime, animation.keyframes[currentKeyframe], animation.keyframes[currentKeyframe + 1])
        );
    }

    if (animation.channels.test((int)Animation::Channel::Rotation)) {
        GetTransform()->SetRotation(
            Keyframe::InterpolateRotation(currentTime, animation.keyframes[currentKeyframe], animation.keyframes[currentKeyframe + 1])
        );
    }

    if (animation.channels.test((int)Animation::Channel::Scale)) {
        GetTransform()->SetScale(
            Keyframe::InterpolateScale(currentTime, animation.keyframes[currentKeyframe], animation.keyframes[currentKeyframe + 1])
        );
    }

    float nextTime = animation.keyframes[currentKeyframe + 1].timestamp;
    if (currentTime > nextTime)
        currentKeyframe++;
}
