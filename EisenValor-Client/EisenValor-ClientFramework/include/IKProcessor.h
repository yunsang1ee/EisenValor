#pragma once
#include <DirectXMath.h>
#include <vector>
#include <string>

// IK 대상 정보 (예: 오른손의 위치를 특정 좌표로 이동)
struct IKTarget {
    uint32_t boneIndex;          // 손목 (End Effector, 예: Hand_R)
    uint32_t midBoneIndex;       // 팔꿈치 (Joint, 예: Elbow_R)
    uint32_t rootBoneIndex;      // 어깨 (Root, 예: Shoulder_R)
    
    DirectX::XMVECTOR targetPos; // 목표 모델 공간 좌표 (Model Space)
    DirectX::XMVECTOR poleVector;// 팔꿈치가 향할 방향 (힌트, Elbow Hint)
    float weight = 0.0f;         // IK 적용 강도 (0: 미적용, 1: 완전 적용)
    
    bool active = false;
};

class IKProcessor {
public:
    IKProcessor() = default;
    ~IKProcessor() = default;

    // Two-Bone IK 계산 및 전역 행렬 수정
    void SolveTwoBoneIK(
        std::vector<DirectX::XMFLOAT4X4>& globalMatrices,
        const std::vector<DirectX::XMFLOAT4X4>& preIKGlobalMatrices,
        const IKTarget& target
    );
};
