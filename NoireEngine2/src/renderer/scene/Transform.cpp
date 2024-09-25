#include "Transform.hpp"

#include "Entity.hpp"
#include "editor/ImGuiExtension.hpp"

Transform::Transform(const Transform& other) : 
    m_Position(other.m_Position), m_Rotation(other.m_Rotation), m_Scale(other.m_Scale), parent(other.parent) {
}

Transform::Transform(glm::vec3 t) : 
    m_Position(t) {
}

Transform::Transform(glm::vec3 t, glm::vec3 euler) :
    m_Position(t), m_Rotation(euler)
{
}

Transform::Transform(glm::vec3 t, glm::vec3 euler, glm::vec3 s) : 
    m_Position(t), m_Rotation(euler), m_Scale(s) {
}

Transform::Transform(glm::vec3 t, glm::quat q, glm::vec3 s) : 
    m_Position(t), m_Rotation(q), m_Scale(s) {
}

Transform::Transform(glm::vec3 t, glm::quat q) :
    m_Position(t), m_Rotation(q) {
}

void Transform::SetPosition(glm::vec3 newPosition)
{
    m_Position.x = newPosition.x;
    m_Position.y = newPosition.y;
    m_Position.z = newPosition.z;

    isDirty = true;
}

void Transform::SetPosition(glm::vec3& newPosition)
{
    m_Position.x = newPosition.x;
    m_Position.y = newPosition.y;
    m_Position.z = newPosition.z;

    isDirty = true;
}

void Transform::SetRotation(glm::quat newRotation)
{
    m_Rotation.x = newRotation.x;
    m_Rotation.y = newRotation.y;
    m_Rotation.z = newRotation.z;
    m_Rotation.w = newRotation.w;

    isDirty = true;
}

void Transform::SetRotation(glm::quat& newRotation)
{
    m_Rotation.x = newRotation.x;
    m_Rotation.y = newRotation.y;
    m_Rotation.z = newRotation.z;
    m_Rotation.w = newRotation.w;

    isDirty = true;
}

void Transform::SetScale(glm::vec3 newScale)
{
    m_Scale.x = newScale.x;
    m_Scale.y = newScale.y;
    m_Scale.z = newScale.z;

    isDirty = true;
}

void Transform::SetScale(glm::vec3& newScale)
{
    m_Scale.x = newScale.x;
    m_Scale.y = newScale.y;
    m_Scale.z = newScale.z;

    isDirty = true;
}

// TODO: move all above to Vector3 or sth
////////////////////////////////////////////////////////////////////////////////////////////////

glm::mat4 Transform::Local() const {
    return glm::translate(Mat4::Identity, m_Position) * glm::mat4_cast(m_Rotation) * glm::scale(Mat4::Identity, m_Scale);
}

glm::mat4 Transform::LocalInverse() const {
    return glm::scale(Mat4::Identity, 1.0f / m_Scale) * glm::mat4_cast(glm::inverse(m_Rotation)) * glm::translate(Mat4::Identity, -m_Position);
}

glm::vec3 Transform::LocalInverseLocation() const { return -m_Position; }

glm::vec3 Transform::LocalInverseScale() const { return 1.0f / m_Scale; }

glm::quat Transform::LocalInverseRotation() const { return glm::inverse(m_Rotation); }

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
        return parent->WorldLocation() + m_Position;
    }
    else {
        return m_Position;
    }
}

glm::vec3 Transform::WorldScale() const
{
    if (parent) {
        return parent->WorldScale() * m_Scale;
    }
    else {
        return m_Scale;
    }
}

glm::quat Transform::WorldRotation() const
{
    if (parent) {
        return parent->WorldRotation() * m_Rotation;
    }
    else {
        return m_Rotation;
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
    return a.parent != b.parent || a.m_Position != b.m_Position ||
        a.m_Rotation != b.m_Rotation || a.m_Scale != b.m_Scale;
}

glm::vec3 Transform::Forward() const { return m_Rotation * Vec3::Forward; }
glm::vec3 Transform::Back() const { return m_Rotation * Vec3::Back; }
glm::vec3 Transform::Right() const { return m_Rotation * Vec3::Right; }
glm::vec3 Transform::Left() const { return m_Rotation * Vec3::Left; }
glm::vec3 Transform::Up() const { return m_Rotation * Vec3::Up; }
glm::vec3 Transform::Down() const { return m_Rotation * Vec3::Down; }

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
    Decompose(transformation * Local(), m_Position, m_Rotation, m_Scale);
}

glm::quat Transform::Rotation(const float& angle, const glm::vec3& axis, bool useRadians)
{
    return glm::angleAxis(useRadians ? angle : glm::radians(angle), axis);
}

void Transform::Rotate(const float& angle, const glm::vec3& axis, bool useRadians)
{
    m_Rotation = Rotation(angle, axis, useRadians) * m_Rotation;
}

void Transform::RotateAround(const glm::vec3& point, const float& angle, const glm::vec3& axis)
{
    RotateAround(point, angle, Rotation(angle, axis));
}

void Transform::RotateAround(const glm::vec3& point, const float& angle, const glm::quat& rot)
{
    m_Position = rot * (m_Position - point) + point;
    m_Rotation = rot * m_Rotation;
}

void Transform::Translate(const glm::vec3& translation, bool useWorldspace)
{
    if (!useWorldspace)
        m_Position += translation;
    else {
        m_Position += WorldRotation() * translation;
    }
}

glm::vec3 lerp(glm::vec3& x, const glm::vec3& y, float t) {
    return x * (1.f - t) + y * t;
}

void Transform::LerpTo(const glm::vec3& targetPosition, float t)
{
    m_Position = lerp(m_Position, targetPosition, t);
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

void Transform::Inspect()
{
    static const char* labels[] = { "X", "Y", "Z" };

    ImGuiExt::DrawVec3(m_Position, "Position", labels);
    ImGui::Separator(); // --------------------------------------------------
    ImGuiExt::DrawQuaternion(m_Rotation, "Rotation", labels);
    ImGui::Separator(); // --------------------------------------------------
    ImGuiExt::DrawVec3(m_Scale, "Scale", labels);
}