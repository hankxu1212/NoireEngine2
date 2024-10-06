//
// Created by Hank Xu on 2/17/24.
//

#pragma once

#include "Frustum.hpp"
#include "renderer/scene/Transform.hpp"

class TransformComponent;

class Camera 
{
public:
    enum Type { Game, Scene, Debug, Other };

    Camera();
    Camera(const Camera& other) = default;
    Camera(Type type_);
    Camera(Type type_, bool orthographic_, float orthographicScale_, float np, float fp); // for creating orthographic cameras
    Camera(Type type_, bool orthographic_, float np, float fp, float fov, float aspect); // for creating perspective cameras
    ~Camera() = default;

    /**
      * Updates view and projection matrix based on the transform the camera is attached to
    */
    void Update(const Transform& transform);

    inline const glm::mat4& getViewMatrix() const { return viewMatrix; }
    inline const glm::mat4& getProjectionMatrix() const { return projectionMatrix; }
    inline const glm::mat4& getWorldToClipMatrix() const { return worldToClip; }

    inline glm::mat4& GetViewMatrixUnsafe() { return viewMatrix; } // allows function to modify the projection matrix. use with care!
    inline glm::mat4& GetProjectionMatrixUnsafe() { return projectionMatrix; } // allows function to modify the projection matrix. use with care!
    
    inline const Frustum& getViewFrustum() const { return frustum; }
    inline Type getType() const { return type; }
    const char* getTypeStr() const;

    float nearClipPlane = 0.1f;
    float farClipPlane = 1000.0f;
    float fieldOfView = 1.0471975512f; // around 60 
    float aspectRatio = 1980.0f/1020.0f;

    bool orthographic = false;
    float orthographicScale = 5;

private:

    Type type = Scene;

    //glm::vec3 velocity; // This camera's motion in units per second as it was during the last frame.

    Frustum frustum;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::mat4 worldToClip; // proj * view
};
