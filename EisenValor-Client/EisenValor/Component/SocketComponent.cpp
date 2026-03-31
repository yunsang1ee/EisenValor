#include "stdafxClient.h"
#include "SocketComponent.h"
#include "GameObject.h"
#include "GameObject.inl"
#include "AnimationComponent.h"
#include <DirectXMath.h>

using namespace DirectX;

void SocketComponent::OnLateUpdate(float dt)
{
    if (!m_targetActor || m_boneIndex == 0xFFFFFFFF)
        return;

    auto* animComp = m_targetActor->GetComponent<AnimationComponent>();
    if (!animComp)
        return;

    XMMATRIX boneMatrix;
    if (animComp->GetSocketMatrix(m_boneIndex, boneMatrix))
    {
        // 오프셋 행렬 로드
        XMMATRIX offset = XMLoadFloat4x4(&m_offsetMatrix);

        // 최종 소켓 본의 로컬 행렬 계산 (Offset * Bone Local Matrix)
        XMMATRIX socketBoneLocal = offset * boneMatrix;

        // Transform 성분 분해 및 주입 (Local Space)
        XMVECTOR s, r, t;
        if (XMMatrixDecompose(&s, &r, &t, socketBoneLocal))
        {
            auto& tr = GetGameObject()->GetTransform();
            
            XMFLOAT3 pos, scale;
            XMFLOAT4 rot;
            XMStoreFloat3(&pos, t);
            XMStoreFloat3(&scale, s);
            XMStoreFloat4(&rot, r);

            tr.SetPosition(pos);
            tr.SetScale(scale.x); // Uniform Scale 가정
            tr.SetRotationQuaternion(rot);
        }
    }
}

void SocketComponent::SetTarget(GameObject* targetActor, const std::string& socketName)
{
    m_targetActor = targetActor;
    m_socketName = socketName;
    m_boneIndex = 0xFFFFFFFF;

    if (m_targetActor)
    {
        auto* animComp = m_targetActor->GetComponent<AnimationComponent>();
        if (animComp)
        {
            if (!animComp->GetBoneIndexByName(m_socketName, m_boneIndex))
            {
                DEBUG_LOG_FMT("[SocketComponent] Failed to find bone: {} in Actor: {}\n", m_socketName.c_str(), m_targetActor->GetName().c_str());
            }
            else
            {
                DEBUG_LOG_FMT("[SocketComponent] Successfully found bone: {} (Index: {}) in Actor: {}\n", m_socketName.c_str(), m_boneIndex, m_targetActor->GetName().c_str());
            }
        }
        else
        {
             DEBUG_LOG_FMT("[SocketComponent] Actor: {} has no AnimationComponent\n", m_targetActor->GetName().c_str());
        }
    }
}
