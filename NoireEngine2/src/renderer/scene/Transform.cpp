#include "Transform.hpp"

#include "Entity.hpp"
#include "editor/ImGuiExtension.hpp"

Transform::Transform(const Transform& other) : 
    m_Position(other.m_Position), m_Rotation(other.m_Rotation), m_Scale(other.m_Scale), m_Parent(other.m_Parent) {
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
    m_Position = newPosition;
    isDirty = true;
}

void Transform::SetPosition(glm::vec3& newPosition)
{
    m_Position = newPosition;
    isDirty = true;
}

void Transform::SetRotation(glm::quat newRotation)
{
    m_Rotation = newRotation;
    isDirty = true;
}

void Transform::SetRotation(glm::quat& newRotation)
{
    m_Rotation = newRotation;
    isDirty = true;
}

void Transform::SetScale(glm::vec3 newScale)
{
    m_Scale = newScale;
    isDirty = true;
}

void Transform::SetScale(glm::vec3& newScale)
{
    m_Scale = newScale;
    isDirty = true;
}

// TODO: move all above to Vector3 or sth
////////////////////////////////////////////////////////////////////////////////////////////////

glm::mat4 Transform::Local() const {
    return glm::translate(Mat4::Identity, m_Position) * glm::mat4_cast(m_Rotation) * glm::scale(Mat4::Identity, m_Scale);
}

glm::mat4 Transform::LocalDirty()
{
    if (isDirty) {
        localMat = Local();
        isDirty = false;
    }
    return localMat;
}

glm::mat4 Transform::LocalInverse() const {
    return glm::scale(Mat4::Identity, 1.0f / m_Scale) * glm::mat4_cast(glm::inverse(m_Rotation)) * glm::translate(Mat4::Identity, -m_Position);
}

glm::vec3 Transform::LocalInverseLocation() const { return -m_Position; }

glm::vec3 Transform::LocalInverseScale() const { return 1.0f / m_Scale; }

glm::quat Transform::LocalInverseRotation() const { return glm::inverse(m_Rotation); }

glm::mat4 Transform::World() const {
    if (m_Parent) {
        return m_Parent->World() * Local();
    }
    else {
        return Local();
    }
}

glm::vec3 Transform::WorldLocation() const
{
    if (m_Parent) {
        return m_Parent->WorldLocation() + m_Position;
    }
    else {
        return m_Position;
    }
}

glm::vec3 Transform::WorldScale() const
{
    if (m_Parent) {
        return m_Parent->WorldScale() * m_Scale;
    }
    else {
        return m_Scale;
    }
}

glm::quat Transform::WorldRotation() const
{
    if (m_Parent) {
        return m_Parent->WorldRotation() * m_Rotation;
    }
    else {
        return m_Rotation;
    }
}

glm::mat4 Transform::WorldInverse() const {
    if (m_Parent) {
        return LocalInverse() * m_Parent->WorldInverse();
    }
    else {
        return LocalInverse();
    }
}

glm::vec3 Transform::WorldInverseLocation() const { return -WorldLocation(); }

glm::vec3 Transform::WorldInverseScale() const { return 1.0f / WorldScale(); }

glm::quat Transform::WorldInverseRotation() const { return glm::inverse(WorldRotation()); }

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

void Transform::AttachParent(Transform& m_ParentTransform)
{
    m_Parent = &m_ParentTransform;
    auto m_ParentInverse = m_ParentTransform.LocalInverse();
    Apply(m_ParentInverse);
}

void Transform::RemoveParent()
{
    auto m_ParentWorld = m_Parent->World();
    Apply(m_ParentWorld);
}

void Transform::Inspect()
{
    static const char* labels[] = { "X", "Y", "Z" };

    bool valueChanged = false;
    valueChanged |= ImGuiExt::DrawVec3(m_Position, "Position", labels);
    ImGui::Separator(); // --------------------------------------------------
    valueChanged |= ImGuiExt::DrawQuaternion(m_Rotation, "Rotation", labels);
    ImGui::Separator(); // --------------------------------------------------
    valueChanged |= ImGuiExt::DrawVec3(m_Scale, "Scale", labels);

    if (valueChanged)
        isDirty = true;
}