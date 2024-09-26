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

    // Function to animate based on current time
    void Animate();

    std::shared_ptr<Animation> m_Animation;

private:
    float currentTime = 0;
    uint32_t currentKeyframe = 0;
    float m_PlaybackSpeed;
    bool isAnimating = true;
};

