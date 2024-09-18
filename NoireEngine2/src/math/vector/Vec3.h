#pragma once

#include <glm/glm.hpp>

class Vec3
{
public:
    static const glm::vec3 Forward; // the forward vector in absolute world coordinate
    static const glm::vec3 Back; // the back vector in absolute world coordinate
    static const glm::vec3 Right; // the right vector in absolute world coordinate
    static const glm::vec3 Left; // the left vector in absolute world coordinate
    static const glm::vec3 Up; // the up vector in absolute world coordinate
    static const glm::vec3 Down; // the down vector in absolute world coordinate

    static const glm::vec3 Zero; // the zero vector
    static const glm::vec3 One; // the one vector
};

