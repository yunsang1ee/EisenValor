#include "stdafxClient.h"
#include "SocketComponent.h"
#include "GameObject.h"
#include "GameObject.inl"
#include "AnimationComponent.h"
#include <DirectXMath.h>

using namespace DirectX;

void SocketComponent::OnLateUpdate(float dt)
{
	if (!ownerObjhandle.IsValid() || m_boneIndex == 0xFFFFFFFF)
        return;

	auto* socketObj = GetGameObject();
	auto* scene = socketObj ? socketObj->GetScene() : nullptr;
	auto* ownerObj = scene ? scene->TryGetGameObject(ownerObjhandle) : nullptr;
	if (!ownerObj)
        return;

	auto* animComp = ownerObj->GetComponent<AnimationComponent>();
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
			auto& tr = socketObj->GetTransform();
            
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

void SocketComponent::SetTarget(GameObject* ownerObj, const std::string& socketName)
{
	ownerObjhandle = ownerObj ? ownerObj->GetHandle() : HandleOf<GameObject>::Invalid();
    m_socketName = socketName;
    m_boneIndex = 0xFFFFFFFF;

    if (ownerObj)
    {
		auto* animComp = ownerObj->GetComponent<AnimationComponent>();
        if (animComp)
        {
            if (!animComp->GetBoneIndexByName(m_socketName, m_boneIndex))
            {
               DEBUG_LOG_FMT("[SocketComponent] Failed to find bone: {} in Actor: {}\n", m_socketName.c_str(), ownerObj->GetName().c_str());
            }
            else
            {
                //DEBUG_LOG_FMT("[SocketComponent] Successfully found bone: {} (Index: {}) in Actor: {}\n", m_socketName.c_str(), m_boneIndex, ownerObj->GetName().c_str());
            }
        }
        else
        {
             DEBUG_LOG_FMT("[SocketComponent] Actor: {} has no AnimationComponent\n", ownerObj->GetName().c_str());
        }
    }
}
