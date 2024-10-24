#include "AABB.hpp"

void AABB::Update(const glm::mat4& model)
{
    // Get the 8 corners of the local AABB
    std::array<glm::vec3, 8> corners = {
        glm::vec3(originMin.x, originMin.y, originMin.z),
        glm::vec3(originMax.x, originMin.y, originMin.z),
        glm::vec3(originMin.x, originMax.y, originMin.z),
        glm::vec3(originMax.x, originMax.y, originMin.z),
        glm::vec3(originMin.x, originMin.y, originMax.z),
        glm::vec3(originMax.x, originMin.y, originMax.z),
        glm::vec3(originMin.x, originMax.y, originMax.z),
        glm::vec3(originMax.x, originMax.y, originMax.z),
    };

    // Transform all corners
    min = glm::vec3(FLT_MAX);
    max = glm::vec3(-FLT_MAX);
    for (const auto& corner : corners) {
        glm::vec3 transformedCorner = glm::vec3(model * glm::vec4(corner, 1.0f));
        min = glm::min(min, transformedCorner);
        max = glm::max(max, transformedCorner);
    }
}