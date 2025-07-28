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
    virtual void Render(ID3D12GraphicsCommandList* cmdList,
        DirectX::XMMATRIX view,
        DirectX::XMMATRIX projection) = 0;

    // 공통 기능들
    virtual void SetPosition(float x, float y, float z) { m_x = x; m_y = y; m_z = z; }
    virtual DirectX::XMFLOAT3 GetPosition() const { return { m_x, m_y, m_z }; }
    virtual void SetRotation(float yaw) { m_yaw = yaw; }
    virtual float GetRotation() const { return m_yaw; }

    // 오브젝트 타입 구분용
    enum class ObjectType
    {
        Player  //일단 플레이어만
    };

    virtual ObjectType GetObjectType() const = 0;

protected:
    // 기본 Transform 데이터
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_z = 0.0f;
    float m_yaw = 0.0f;
};

