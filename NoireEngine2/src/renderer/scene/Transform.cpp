#include "Transform.hpp"

Transform Transform::base;

// TODO: move all above to Vector3 or sth
////////////////////////////////////////////////////////////////////////////////////////////////

glm::mat4 Transform::Local() const {
    return glm::translate(Mat4::Identity, position) * glm::mat4_cast(rotation) * glm::scale(Mat4::Identity, scale);
}

glm::mat4 Transform::LocalInverse() const {
    return glm::scale(Mat4::Identity, 1.0f / scale) * glm::mat4_cast(glm::inverse(rotation)) * glm::translate(Mat4::Identity, -position);
}

glm::vec3 Transform::LocalInverseLocation() const { return -position; }

glm::vec3 Transform::LocalInverseScale() const { return 1.0f / scale; }

glm::quat Transform::LocalInverseRotation() const { return glm::inverse(rotation); }

glm::mat4 Transform::World() const {
    if (parent) {
        return parent->World() * Local();
    }
    else {
        return Local();
    }
}

glm::vec3 Transform::WorldLocation() const
{
    if (parent) {
        return parent->WorldLocation() + position;
    }
    else {
        return position;
    }
}

glm::vec3 Transform::WorldScale() const
{
    if (parent) {
        return parent->WorldScale() * scale;
    }
    else {
        return scale;
    }
}

glm::quat Transform::WorldRotation() const
{
    if (parent) {
        return parent->WorldRotation() * rotation;
    }
    else {
        return rotation;
    }
}

glm::mat4 Transform::WorldInverse() const {
    if (parent) {
        return LocalInverse() * parent->WorldInverse();
    }
    else {
        return LocalInverse();
    }
}

glm::vec3 Transform::WorldInverseLocation() const { return -WorldLocation(); }

glm::vec3 Transform::WorldInverseScale() const { return 1.0f / WorldScale(); }

glm::quat Transform::WorldInverseRotation() const { return glm::inverse(WorldRotation()); }

bool operator!=(const Transform& a, const Transform& b) {
    return a.parent != b.parent || a.position != b.position ||
        a.rotation != b.rotation || a.scale != b.scale;
}

glm::vec3 Transform::Forward() const { return rotation * Vec3::Forward; }
glm::vec3 Transform::Back() const { return rotation * Vec3::Back; }
glm::vec3 Transform::Right() const { return rotation * Vec3::Right; }
glm::vec3 Transform::Left() const { return rotation * Vec3::Left; }
glm::vec3 Transform::Up() const { return rotation * Vec3::Up; }
glm::vec3 Transform::Down() const { return rotation * Vec3::Down; }

void Transform::Decompose(const glm::mat4& m, glm::vec3& pos, glm::quat& rot, glm::vec3& scale)
{
    pos = m[3];
    for (int i = 0; i < 3; i++)
        scale[i] = glm::length(glm::vec3(m[i]));

    const glm::mat3 rotMtx(
        glm::vec3(m[0]) / scale[0],
        glm::vec3(m[1]) / scale[1],
        glm::vec3(m[2]) / scale[2]);

    rot = glm::quat_cast(rotMtx);
}

void Transform::Apply(glm::mat4& transformation)
{
    Decompose(transformation * Local(), position, rotation, scale);
}

glm::quat Transform::Rotation(const float& angle, const glm::vec3& axis, bool useRadians)
{
    return glm::angleAxis(useRadians ? angle : glm::radians(angle), axis);
}

void Transform::Rotate(const float& angle, const glm::vec3& axis, bool useRadians)
{
    rotation = Rotation(angle, axis, useRadians) * rotation;
}

void Transform::RotateAround(const glm::vec3& point, const float& angle, const glm::vec3& axis)
{
    RotateAround(point, angle, Rotation(angle, axis));
}

void Transform::RotateAround(const glm::vec3& point, const float& angle, const glm::quat& rot)
{
    position = rot * (position - point) + point;
    rotation = rot * rotation;
}

void Transform::Translate(const glm::vec3& translation, bool useWorldspace)
{
    if (!useWorldspace)
        position += translation;
    else {
        position += WorldRotation() * translation;
    }
}

glm::vec3 lerp(glm::vec3& x, const glm::vec3& y, float t) {
    return x * (1.f - t) + y * t;
}

void Transform::LerpTo(const glm::vec3& targetPosition, float t)
{
    position = lerp(position, targetPosition, t);
}

void Transform::AttachParent(Transform& parentTransform)
{
    parent = &parentTransform;
    auto parentInverse = parentTransform.LocalInverse();
    Apply(parentInverse);
}

void Transform::RemoveParent()
{
    auto parentWorld = parent->World();
    Apply(parentWorld);
}