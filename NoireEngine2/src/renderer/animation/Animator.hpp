#pragma once

#include <memory>

#include "renderer/components/Component.hpp"
#include "Animation.hpp"

class Animator : public Component
{
public:

    Animator() = default;
    Animator(std::shared_ptr<Animation> animation);

    void Update() override;

    void Start();

    void Restart();

    void Stop();

    void Inspect() override;

    void Animate();

    std::shared_ptr<Animation> m_Animation;

    const char* getName() override { return "Animator"; }

private:
    float               m_CurrentTime = 0;
    uint32_t            m_CurrentKeyframe = 0;
    float               m_PlaybackSpeed = 1;
    bool                m_IsAnimating = true;
};

