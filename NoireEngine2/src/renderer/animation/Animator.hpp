#pragma once

#include "renderer/components/Component.hpp"
#include "Animation.hpp"

class Animator : public Component
{
public:
    void Update() override;

    void Start();

    void Restart();

    void Stop();

    void Inspect() override;

    // Function to animate based on current time
    void Animate();
    Animation animation;

private:
    float currentTime = 0;
    uint32_t currentKeyframe = 0;
    float m_PlaybackSpeed;
    bool isAnimating = true;
};

