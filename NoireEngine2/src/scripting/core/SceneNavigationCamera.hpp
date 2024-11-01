#pragma once

#include "scripting/Behaviour.hpp"
#include "core/events/KeyEvent.hpp"
#include "core/events/MouseEvent.hpp"
#include "Input.hpp"

class Camera;
class Transform;

namespace Core {

    class SceneNavigationCamera : public Behaviour
    {
    public:
        void Awake() override;
        
        void Start() override;
        
        void Update() override;
        
        void Shutdown() override;
        
        void HandleEvent(Event& event) override;

        void Inspect() override;
        
        const char* getClassName() const { return "SceneNavigationCamera"; }

    private:
        void HandleMovement();

        bool OnMouseScroll(MouseScrolledEvent& e);

    private:
        Transform* transform;
        Camera* nativeCamera;

        // input keymaps
        InputTypeVec3 planeNavKeyBindings = {
            Key::Q,Key::E,
            Key::A,Key::D,
            Key::S,Key::W,
        };

        MouseCode MOUSE_anchorMouseLeft = Mouse::ButtonLeft;
        MouseCode MOUSE_anchor = Mouse::ButtonMiddle;
        KeyCode KEY_anchor = Key::LeftAlt;

        MouseCode MOUSE_move = Mouse::ButtonRight;

        float moveSpeed = 20;
        float anchoredRotationSensitivity = 300.0f;
        float zoomSpeed = 1.0f;
        float anchoredMoveSensitivity = 13.0f;

        glm::vec3 anchorPoint = Vec3::Zero;
        const glm::vec3 anchorOffset = glm::vec3(0.001f, 0.001f, 0.001f);
        glm::vec3 anchorDir; 

        const float minimumRadius = 0.1f;
        float radius;
    };
}
