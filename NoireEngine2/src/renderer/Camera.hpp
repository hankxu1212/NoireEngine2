//
// Created by Hank Xu on 2/17/24.
//

#pragma once

#include "Frustum.hpp"
#include "renderer/scene/Transform.hpp"

class TransformComponent;

class Camera {
public:
    enum Type { Game, Scene, Preview, Reflection, Light };

    Camera();
    Camera(const Camera& other) = default;
    Camera(Type type_, bool orthographic_, float orthographicScale_, float np, float fp);
    ~Camera() = default;

    /**
      * Updates view and projection matrix based on the transform the camera is attached to
    */
    void Update(const Transform& transform);

    inline const glm::mat4& getViewMatrix() const { return viewMatrix; }
    inline const glm::mat4& getProjectionMatrix() const { return projectionMatrix; }
    inline glm::mat4& GetProjectionMatrixUnsafe() { return projectionMatrix; } // allows function to modify the projection matrix. use with care!
    inline const glm::vec3& getPosition() const { return position; }
    inline const Frustum& getViewFrustum() const { return frustum; }
    const char* getTypeStr() const;


    float nearClipPlane = 0.1f;
    float farClipPlane = 1000.0f;
    float fieldOfView = 60.0f;

    float screenWidth = -1;
    float screenHeight = -1;
    float aspectRatio = -1;

    bool orthographic = false;
    float orthographicScale = 5;

private:
    Type type = Scene;

    //glm::vec3 velocity; // This camera's motion in units per second as it was during the last frame.

    Frustum frustum;

    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::vec3 position;
};
