#include "Animator.hpp"
#include "utils/Logger.hpp"
#include "core/Time.hpp"
#include "renderer/scene/Scene.hpp"
#include "imgui/imgui.h"
#include "core/Core.hpp"

Animator::Animator(std::shared_ptr<Animation> animation) :
    m_Animation(animation)
{
}

void Animator::Update()
{
    if (m_IsAnimating)
        Animate();
}

void Animator::Start()
{
    size_t numKeyframes = m_Animation->keyframes.size();
    if (numKeyframes < 2) {
        NE_ERROR("Error: At least two keyframes are required for m_Animation->");
        return;
    }
    m_IsAnimating = true;
}

void Animator::Restart()
{
    m_CurrentTime = 0;
    m_CurrentKeyframe = 0;
}

void Animator::Stop()
{
    m_IsAnimating = false;
}

void Animator::Inspect()
{
    ImGui::PushID("Animator Inspect");
    {
        ImGui::Columns(2);
        ImGui::Text("Animation Name");
        ImGui::NextColumn();
        if (m_Animation)
            ImGui::Text(m_Animation->m_Name.c_str());
        else
            ImGui::Text(NE_NULL_STR);
        ImGui::Columns(1);

        ImGui::Columns(2);
        ImGui::Text("Start");
        ImGui::NextColumn();
        if (ImGui::Button("##Start", { 20, 20 }))
            Start();
        ImGui::Columns(1);

        ImGui::Columns(2);
        ImGui::Text("Restart");
        ImGui::NextColumn();
        if (ImGui::Button("##Restart", {20, 20}))
            Restart();
        ImGui::Columns(1);

        ImGui::Columns(2);
        ImGui::Text("Stop");
        ImGui::NextColumn();
        if (ImGui::Button("##Stop", { 20, 20 }))
            Stop();
        ImGui::Columns(1);
    }
    ImGui::PopID();
}

void Animator::Animate()
{
    m_CurrentTime += Time::DeltaTime * m_PlaybackSpeed;
    if (m_CurrentTime > m_Animation->duration)
        m_CurrentTime -= m_Animation->duration;

    if (m_Animation->m_Channels.test((uint8_t)Animation::Channel::Position)) {
        GetTransform()->SetPosition(
            Keyframe::InterpolatePosition(m_CurrentTime, m_Animation->keyframes[m_CurrentKeyframe], m_Animation->keyframes[m_CurrentKeyframe + 1])
        );
    }

    if (m_Animation->m_Channels.test((uint8_t)Animation::Channel::Rotation)) {
        GetTransform()->SetRotation(
            Keyframe::InterpolateRotation(m_CurrentTime, m_Animation->keyframes[m_CurrentKeyframe], m_Animation->keyframes[m_CurrentKeyframe + 1])
        );
    }

    if (m_Animation->m_Channels.test((uint8_t)Animation::Channel::Scale)) {
        GetTransform()->SetScale(
            Keyframe::InterpolateScale(m_CurrentTime, m_Animation->keyframes[m_CurrentKeyframe], m_Animation->keyframes[m_CurrentKeyframe + 1])
        );
    }

    float nextTime = m_Animation->keyframes[m_CurrentKeyframe + 1].timestamp;
    if (m_CurrentTime > nextTime)
        m_CurrentKeyframe++;
}

template<>
void Scene::OnComponentAdded<Animator>(Entity& entity, Animator& component)
{
}

template<>
void Scene::OnComponentRemoved<Animator>(Entity& entity, Animator& component)
{
}