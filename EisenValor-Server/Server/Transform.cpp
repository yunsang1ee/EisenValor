#include "pch.h"
#include "Transform.h"

Transform::Transform()
    : m_position{ 0.f, 0.f, 0.f }
    , m_prevPosition{ 0.f, 0.f, 0.f }
    , m_rotation{ 0.f, 0.f, 0.f }
{
}

Transform::Transform(const Vec3& pos, const Vec3& rotDegree)
    : m_position{ pos }
    , m_prevPosition{ pos }  // 스폰 시 prevPosition = position → 첫 틱 GetMovedDistanceSq() == 0
    , m_rotation{ Deg2Rad(rotDegree.x), Deg2Rad(rotDegree.y), Deg2Rad(rotDegree.z) }
{
}

void Transform::SetRotation(const Vec3& rotDegree)
{
    m_rotation = { Deg2Rad(rotDegree.x), Deg2Rad(rotDegree.y), Deg2Rad(rotDegree.z) };
}

Vec3 Transform::GetRotationDegree() const
{
    return { Rad2Deg(m_rotation.x), Rad2Deg(m_rotation.y), Rad2Deg(m_rotation.z) };
}

Vec3 Transform::GetForward() const
{
    const float yaw{ m_rotation.y };
    return { sinf(yaw), 0.f, cosf(yaw) };
}

Vec3 Transform::GetRight() const
{
    const float yaw{ m_rotation.y };
    return { cosf(yaw), 0.f, -sinf(yaw) };
}

Vec3 Transform::GetDeltaPosition() const
{
    return {
        m_position.x - m_prevPosition.x,
        m_position.y - m_prevPosition.y,
        m_position.z - m_prevPosition.z
    };
}

float Transform::GetMovedDistanceSq() const
{
    const Vec3 delta = GetDeltaPosition();
    return delta.x * delta.x + delta.z * delta.z;
}

float Transform::DistanceSq(const Vec3& other) const
{
    const float dx = m_position.x - other.x;
    const float dz = m_position.z - other.z;
    return dx * dx + dz * dz;
}

float Transform::DistanceSq(const Transform& other) const
{
    return DistanceSq(other.GetPosition());
}

float Transform::DistanceTo(const Vec3& other) const
{
    return sqrtf(DistanceSq(other));
}

float Transform::DistanceTo(const Transform& other) const
{
    return sqrtf(DistanceSq(other));
}

bool Transform::IsWithinRange(const Transform& other, const float range) const
{
    return DistanceSq(other) <= range * range;
}

bool Transform::IsWithinRangeSq(const Transform& other, const float rangeSq) const
{
    return DistanceSq(other) <= rangeSq;
}

void Transform::LookAt(const Vec3& target)
{
    //const float dx{ target.x - m_position.x };
    //const float dz{ target.z - m_position.z };
    //if(fabsf(dx) < 0.0001f && fabsf(dz) < 0.0001f)
    //    return;

    //// XZ 평면 기준 Yaw만 회전 (Pitch 무시)
    //m_rotation.y = atan2f(dx, dz);
    const float dx{ target.x - m_position.x };
    const float dz{ target.z - m_position.z };
    if(fabsf(dx) < 0.0001f && fabsf(dz) < 0.0001f)
        return;

    m_targetYaw = atan2f(dx, dz);
    m_isTurning = true;

}

bool Transform::UpdateRotation(const float dt, const float speed)
{
    if(!m_isTurning) return true;

    float diff = m_targetYaw - m_rotation.y;
    while(diff > DirectX::XM_PI) diff -= 2.f * DirectX::XM_PI;
    while(diff < -DirectX::XM_PI) diff += 2.f * DirectX::XM_PI;

    const float delta = diff * dt * speed;

    if(fabsf(delta) >= fabsf(diff)) {
        m_rotation.y = m_targetYaw;
        m_isTurning = false;
        return true;
    }

    m_rotation.y += delta;
    return false;
}
