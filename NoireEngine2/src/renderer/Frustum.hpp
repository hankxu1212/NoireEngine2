#pragma once

#include "glm/glm.hpp"

#include <array>

class Frustum {
public:
    Frustum() = default;

    Frustum(const Frustum& other);

    /**
     * Updates a frustum from the view and projection matrix.
     * @param view The view matrix.
     * @param projection The projection matrix.
     */
    void Update(const glm::mat4& view, const glm::mat4& projection);

    /**
     * Gets if a point contained in the frustum.
     * @param position The point.
     * @return If the point is contained.
     */
    bool PointInFrustum(const glm::vec3& position) const;

    /**
     * Gets if a sphere contained in the frustum.
     * @param position The spheres position.
     * @param radius The spheres radius.
     * @return If the sphere is contained.
     */
    bool SphereInFrustum(const glm::vec3& position, float radius) const;

    /**
     * Gets if a cube contained in the frustum.
     * @param min The cube min point.
     * @param max The cube max point.
     * @return If cube sphere is contained.
     */
    bool CubeInFrustum(const glm::vec3& min, const glm::vec3& max) const;

private:
    void NormalizePlane(int32_t side);
private:
    std::array<std::array<float, 4>, 6> frustum = {};
};