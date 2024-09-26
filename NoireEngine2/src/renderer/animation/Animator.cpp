#include "Animator.hpp"
#include "utils/Logger.hpp"
#include "core/Time.hpp"
#include "renderer/scene/Scene.hpp"

Animator::Animator(std::shared_ptr<Animation> animation) :
    m_Animation(animation)
{
}

void Animator::Update()
{
    //if (isAnimating)
    //    Animate();
}

void Animator::Start()
{
    size_t numKeyframes = m_Animation->keyframes.size();
    if (numKeyframes < 2) {
        NE_ERROR("Error: At least two keyframes are required for m_Animation->");
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

    if (m_Animation->channels.test((int)Animation::Channel::Position)) {
        GetTransform()->SetPosition(
            Keyframe::InterpolatePosition(currentTime, m_Animation->keyframes[currentKeyframe], m_Animation->keyframes[currentKeyframe + 1])
        );
    }

    if (m_Animation->channels.test((int)Animation::Channel::Rotation)) {
        GetTransform()->SetRotation(
            Keyframe::InterpolateRotation(currentTime, m_Animation->keyframes[currentKeyframe], m_Animation->keyframes[currentKeyframe + 1])
        );
    }

    if (m_Animation->channels.test((int)Animation::Channel::Scale)) {
        GetTransform()->SetScale(
            Keyframe::InterpolateScale(currentTime, m_Animation->keyframes[currentKeyframe], m_Animation->keyframes[currentKeyframe + 1])
        );
    }

    float nextTime = m_Animation->keyframes[currentKeyframe + 1].timestamp;
    if (currentTime > nextTime)
        currentKeyframe++;
}

template<>
void Scene::OnComponentAdded<Animator>(Entity& entity, Animator& component)
{
}

template<>
void Scene::OnComponentRemoved<Animator>(Entity& entity, Animator& component)
{
}