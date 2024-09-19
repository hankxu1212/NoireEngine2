#pragma once

#include "scripting/Behaviour.hpp"
#include "core/input/NativeInput.hpp"

namespace Core 
{
    class Input : public Behaviour
    {
    private:
        static Input* g_Instance;
    public:
        Input();
        Input(const Input& cam) = delete;

        static Input* Get();

        void Start() override;
        void Update() override;

        const char* getClassName() const override { return "Input"; }

        glm::vec2 GetMouseDelta() const;
    private:
        glm::vec2 lastMouseScreenPos;
    };
}