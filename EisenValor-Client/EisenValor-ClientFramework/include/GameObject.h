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

	virtual ObjectType GetObjectType() const = 0;

	uint32 m_id;
	bool   alive{true};

protected:
	// 기본 Transform 데이터
	Vec3 m_pos;
	Vec3 m_rot;
};
