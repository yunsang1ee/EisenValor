#pragma once
#include "stdafxClientFramework.h"
#include "DxCommon.h"

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
	virtual void SetPosition(const Vec3& pos) { m_pos = pos; }
	virtual Vec3 GetPosition() const { return m_pos; }
	virtual void SetRotation(const Vec3& rot) { m_rot = rot; }
	virtual Vec3 GetRotation() const { return m_rot; }
	virtual void SetVelocity(const Vec3& velocity) { m_velocity = velocity; }
	virtual Vec3 GetVelocity() const { return m_velocity; }
	virtual void SetAccelration(const Vec3& acceleration) { m_acceleration = acceleration; }
	virtual Vec3 GetAcceleration() const { return m_acceleration; }

	virtual ObjectType GetObjectType() const = 0;
	
	// 팀 구분용
	enum class Team
	{
		BLUE,
		RED
	};

public:
	void Handle_SC_MOVE(
		const Vec3& pos, const Vec3& rot, const Vec3& velocity, const Vec3& accel, const uint64 timeStamp
	);
	// 타입 설정 및 확인
	void SetTeam(Team team);
	Team GetTeam() const { return m_team; }

	virtual void SetTeamColor() {}

public:
	uint32 m_id;
	bool   alive{true};

protected:
	// 기본 Transform 데이터
	Vec3 m_pos{0.f, 0.f, 0.f};
	Vec3 m_rot{0.f, 0.f, 0.f};

	Vec3 m_velocity{0.f, 0.f, 0.f};
	Vec3 m_acceleration{0.f, 0.f, 0.f};
	Team m_team = Team::BLUE;
	Vec4 m_teamColor;

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
