#pragma once
#include "IComponent.h"
#include <DirectXMath.h>
#include <string>

class GameObject;

class SocketComponent : public ComponentBase<SocketComponent>
{
public:
    static constexpr const char* GetStaticTypeName() { return "SocketComponent"; }

    SocketComponent() = default;
    ~SocketComponent() override = default;

    void OnLateUpdate(float dt);

    // 부착 대상 캐릭터와 뼈 이름을 설정합니다.
    // 뼈 정보를 가져올 대상 캐릭터 GameObject
    // 부착할 뼈의 이름 (예: "hand_r")
    void SetTarget(GameObject* targetActor, const std::string& socketName);


    // 무기 자체의 위치/회전 보정값을 설정
    // 오프셋 행렬
    void SetOffsetMatrix(const DirectX::XMMATRIX& offsetMatrix) { DirectX::XMStoreFloat4x4(&m_offsetMatrix, offsetMatrix); }

private:
    GameObject* m_targetActor = nullptr;
    std::string m_socketName;
    uint32_t    m_boneIndex = 0xFFFFFFFF;
    DirectX::XMFLOAT4X4 m_offsetMatrix = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
};
