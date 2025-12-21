#pragma once
#include "stdafxClientFramework.h"
#include "DxCommon.h"
#include "Transform.h" 

class GameObject
{
public:
	GameObject() = default;
	virtual ~GameObject() = default;

	// 모든 게임 오브젝트가 구현해야 하는 순수 가상 함수들
	virtual void Initialize(ID3D12Device* device) = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void Render(ID3D12GraphicsCommandList* cmdList, DirectX::XMMATRIX view, DirectX::XMMATRIX projection) = 0;

	// 공통 기능들
	virtual void SetPosition(const Vec3& pos)
	{
		m_transform.SetPosition(pos); // Transform 업데이트
	}
	virtual Vec3 GetPosition() const
	{
		return m_transform.GetPosition(); // Transform에서 반환
	}
	virtual void SetRotation(const Vec3& rot)
	{
		m_transform.SetRotation(rot); // Transform
	}
	virtual Vec3 GetRotation() const
	{
		return m_transform.GetRotation(); // Transform에서 반환
	}
	virtual void SetVelocity(const Vec3& velocity) { m_velocity = velocity; }
	virtual Vec3 GetVelocity() const { return m_velocity; }
	virtual void SetAccelration(const Vec3& acceleration) { m_acceleration = acceleration; }
	virtual Vec3 GetAcceleration() const { return m_acceleration; }

	virtual FB_ENUMS::GAME_OBJECT_TYPE GetObjectType() const = 0;
	
	// Transform 접근자
	Transform&		 GetTransform() { return m_transform; }
	const Transform& GetTransform() const { return m_transform; }


public:
	void Handle_SC_MOVE(
		const Vec3& pos, const Vec3& rot, const Vec3& velocity, const Vec3& accel, const uint64 timeStamp
	);
	// 타입 설정 및 확인
	void SetTeam(const FB_ENUMS::TEAM_TYPE team);
	FB_ENUMS::TEAM_TYPE GetTeam() const { return m_team; }

	virtual void SetTeamColor() {}

	 // HP 관련 함수들
	void   SetCurrentHP(uint32 hp) { m_currentHP = hp; }
	void   SetMaxHP(uint32 hp) { m_maxHP = hp; }
	uint32 GetCurrentHP() const { return m_currentHP; }
	uint32 GetMaxHP() const { return m_maxHP; }
	float  GetHPRatio() const { return static_cast<float>(m_currentHP) / static_cast<float>(m_maxHP); }

public:
	uint32 m_id;
	bool   alive{true};

protected:
	Vec3 m_velocity{0.f, 0.f, 0.f};
	Vec3 m_acceleration{0.f, 0.f, 0.f};
	
	// Transform 컴포넌트
	Transform			m_transform;

	FB_ENUMS::TEAM_TYPE m_team = FB_ENUMS::TEAM_TYPE_BLUE;
	Vec4 m_teamColor;

	// HP 관련 변수 추가 (HP 수정용)
	uint32 m_currentHP = 100;
	uint32 m_maxHP = 100;

public:
	Vec3	lastServerPosition;
	Vec3	lastServerVelocity;	
	Vec3	lastServerAcceleration;
	Vec3	lastServerRotation;
	uint64	lastServerTimestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();

public:
	bool keyup{false};

	public:
	Vec3 SmoothLerp(const Vec3& curPos, const Vec3& destPos, const float lerpFactor);
};
