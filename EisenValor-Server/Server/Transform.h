#pragma once

class Transform {
public:
    Transform();
    explicit Transform(const Vec3& pos, const Vec3& rotDegree);
    ~Transform() = default;

public:
    void        SetPosition(const Vec3& pos) { m_position = pos; }
    const Vec3& GetPosition() const { return m_position; }

    void        SetRotation(const Vec3& rotDegree);
    const Vec3& GetRotation() const { return m_rotation; }   // Radian
    Vec3        GetRotationDegree() const;

    void  SetYaw(const float yawDegree) { m_rotation.y = Deg2Rad(yawDegree); }
    float GetYaw() const { return m_rotation.y; }          // Radian
    float GetYawDegree() const { return Rad2Deg(m_rotation.y); } // Degree

    Vec3 GetForward() const;
    Vec3 GetRight()   const;
    Vec3 GetBack()    const { const Vec3 f = GetForward(); return { -f.x, 0.f, -f.z }; }
    Vec3 GetLeft()    const { const Vec3 r = GetRight();   return { -r.x, 0.f, -r.z }; }

    const Vec3& GetPrevPosition()    const { return m_prevPosition; }
    Vec3        GetDeltaPosition()   const;
    float       GetMovedDistanceSq() const;

    float DistanceSq(const Vec3& other)      const;
    float DistanceSq(const Transform& other) const;
    float DistanceTo(const Vec3& other)      const;
    float DistanceTo(const Transform& other) const;

    bool IsWithinRange(const Transform& other, const float range)   const;  
    bool IsWithinRangeSq(const Transform& other, const float rangeSq) const;

    void CommitPosition() { m_prevPosition = m_position; }
    void LookAt(const Vec3& target);

    bool  UpdateRotation(const float dt, const float speed = 5.f); 
    bool  IsTurning() const { return m_isTurning; }

private:
    Vec3 m_position;
    Vec3 m_prevPosition;
    Vec3 m_rotation;    // Radian

    float m_targetYaw = 0.f;
    bool  m_isTurning = false;
};