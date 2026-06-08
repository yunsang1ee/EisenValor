#include "stdafxClientFramework.h"
#include "IKProcessor.h"
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <unordered_map>

using namespace DirectX;

// 헬퍼 함수
// 두 정규화된 벡터 사이의 회전 쿼터니언 구하기
static XMVECTOR GetRotationBetweenVectors(XMVECTOR from, XMVECTOR to)
{
    from = XMVector3Normalize(from);
    to = XMVector3Normalize(to);

    float dot = XMVectorGetX(XMVector3Dot(from, to));
    if (dot > 0.9999f) return XMQuaternionIdentity();
    if (dot < -0.9999f)
    {
        XMVECTOR axis = XMVector3Normalize(XMVector3Cross(from, XMVectorSet(1, 0, 0, 0)));
        if (XMVectorGetX(XMVector3Length(axis)) < 0.1f)
            axis = XMVector3Normalize(XMVector3Cross(from, XMVectorSet(0, 1, 0, 0)));
        return XMQuaternionRotationAxis(axis, XM_PI);
    }

    XMVECTOR axis = XMVector3Normalize(XMVector3Cross(from, to));
    float angle = std::acos(std::clamp(dot, -1.0f, 1.0f));
    return XMQuaternionRotationAxis(axis, angle);
}

// 행렬 선형 보간
static XMMATRIX MatrixLerp(CXMMATRIX M1, CXMMATRIX M2, float t)
{
    XMMATRIX res;
    res.r[0] = XMVectorLerp(M1.r[0], M2.r[0], t);
    res.r[1] = XMVectorLerp(M1.r[1], M2.r[1], t);
    res.r[2] = XMVectorLerp(M1.r[2], M2.r[2], t);
    res.r[3] = XMVectorLerp(M1.r[3], M2.r[3], t);
    return res;
}

static uint64_t MakeIKBendCacheKey(const std::vector<XMFLOAT4X4>& globalMatrices, const IKTarget& target)
{
    const auto matrixAddress = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(globalMatrices.data()));
    return matrixAddress ^
        (static_cast<uint64_t>(target.rootBoneIndex) << 32) ^
        (static_cast<uint64_t>(target.midBoneIndex) << 16) ^
        static_cast<uint64_t>(target.boneIndex);
}

void IKProcessor::SolveTwoBoneIK(
    std::vector<XMFLOAT4X4>& globalMatrices,
    const std::vector<XMFLOAT4X4>& preIKGlobalMatrices,
    const IKTarget& target
)
{
    if (!target.active || target.weight <= 0.0f)
        return;

    // 각 본의 현재 월드 행렬
    XMMATRIX rootWorld = XMLoadFloat4x4(&globalMatrices[target.rootBoneIndex]);
    XMMATRIX midWorld = XMLoadFloat4x4(&globalMatrices[target.midBoneIndex]);
    XMMATRIX endWorld = XMLoadFloat4x4(&globalMatrices[target.boneIndex]);
    XMMATRIX preIKRootWorld = XMLoadFloat4x4(&preIKGlobalMatrices[target.rootBoneIndex]);
    XMMATRIX preIKMidWorld = XMLoadFloat4x4(&preIKGlobalMatrices[target.midBoneIndex]);
    XMMATRIX preIKEndWorld = XMLoadFloat4x4(&preIKGlobalMatrices[target.boneIndex]);

    // 각 관절의 월드 위치
    XMVECTOR rootPos = rootWorld.r[3];
    XMVECTOR midPos = midWorld.r[3];
    XMVECTOR endPos = endWorld.r[3];
    XMVECTOR targetPos = target.targetPos;
    XMVECTOR preIKRootPos = preIKRootWorld.r[3];
    XMVECTOR preIKMidPos = preIKMidWorld.r[3];
    XMVECTOR preIKEndPos = preIKEndWorld.r[3];

    // 본의 길이 계산
    float a = XMVectorGetX(XMVector3Length(XMVectorSubtract(midPos, rootPos))); 
    float b = XMVectorGetX(XMVector3Length(XMVectorSubtract(endPos, midPos)));  
    float c = XMVectorGetX(XMVector3Length(XMVectorSubtract(targetPos, rootPos))); 

    // 거리 제약 (팔을 너무 펴거나 굽히지 않도록)
    c = std::clamp(c, std::abs(a - b) + 0.001f, a + b - 0.001f);

    // 코사인 법칙으로 무릎이 얼마나 접혀야 하는지 구하기
    // 어깨 각도 구하기
    float cosA = (a * a + c * c - b * b) / (2.0f * a * c);
    cosA = std::clamp(cosA, -1.0f, 1.0f);
    float angleA = std::acos(cosA);

    // IK 적용 전 원본 애니메이션의 무릎 방향을 평면 기준으로 쓰기
    XMVECTOR targetDir = XMVector3Normalize(XMVectorSubtract(targetPos, rootPos));
    XMVECTOR currentMidDir = XMVector3Normalize(XMVectorSubtract(midPos, rootPos));
    XMVECTOR preIKRootToMid = XMVectorSubtract(preIKMidPos, preIKRootPos);
    XMVECTOR preIKMidToEnd = XMVectorSubtract(preIKEndPos, preIKMidPos);
    XMVECTOR preIKMidDir = XMVector3Normalize(preIKRootToMid);
    XMVECTOR bendDir = XMVectorSubtract(
        preIKMidDir,
        XMVectorScale(targetDir, XMVectorGetX(XMVector3Dot(preIKMidDir, targetDir)))
    );
    XMVECTOR preIKBendPlaneNormal = XMVector3Cross(preIKRootToMid, preIKMidToEnd);
    if (XMVectorGetX(XMVector3LengthSq(preIKBendPlaneNormal)) >= 0.000001f)
    {
        preIKBendPlaneNormal = XMVector3Normalize(preIKBendPlaneNormal);
        XMVECTOR planeBendDir = XMVector3Cross(targetDir, preIKBendPlaneNormal);
        if (XMVectorGetX(XMVector3LengthSq(planeBendDir)) >= 0.000001f)
        {
            planeBendDir = XMVector3Normalize(planeBendDir);
            if (XMVectorGetX(XMVector3Dot(planeBendDir, bendDir)) < 0.0f)
            {
                planeBendDir = XMVectorNegate(planeBendDir);
            }
            bendDir = planeBendDir;
        }
    }
    
    // 원본 애니메이션의 무릎 방향을 먼저 기준으로 쓰기
    if (XMVectorGetX(XMVector3LengthSq(bendDir)) < 0.000001f)
    {
        XMVECTOR poleDir = XMVector3Normalize(target.poleVector);
        bendDir = XMVectorSubtract(
            poleDir,
            XMVectorScale(targetDir, XMVectorGetX(XMVector3Dot(poleDir, targetDir)))
        );
    }
	// 그래도 거의 0이면, 목표 방향과 수직인 다른 방향을 대신 쓰기
    if (XMVectorGetX(XMVector3LengthSq(bendDir)) < 0.000001f)
    {
        const XMVECTOR upAxis = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        const XMVECTOR rightAxis = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
        const XMVECTOR fallbackAxis =
            std::abs(XMVectorGetX(XMVector3Dot(targetDir, upAxis))) < 0.95f ? upAxis : rightAxis;
        bendDir = XMVectorSubtract(
            fallbackAxis,
            XMVectorScale(targetDir, XMVectorGetX(XMVector3Dot(fallbackAxis, targetDir)))
        );
    }
    bendDir = XMVector3Normalize(bendDir);

    static std::unordered_map<uint64_t, XMFLOAT3> previousBendDirs;
    const uint64_t bendCacheKey = MakeIKBendCacheKey(globalMatrices, target);
    float bendContinuity = 1.0f;
    if (auto it = previousBendDirs.find(bendCacheKey); it != previousBendDirs.end())
    {
        XMVECTOR previousBendDir = XMLoadFloat3(&it->second);
        if (XMVectorGetX(XMVector3LengthSq(previousBendDir)) >= 0.000001f)
        {
            previousBendDir = XMVector3Normalize(previousBendDir);
            bendContinuity = std::clamp(XMVectorGetX(XMVector3Dot(previousBendDir, bendDir)), -1.0f, 1.0f);
            if (bendContinuity < -0.6f)
            {
				const float bendContinuityAlpha = target.bendContinuityAlpha;
                bendDir = XMVector3Normalize(XMVectorLerp(previousBendDir, bendDir, 0.01f));
            }
        }
    }
    XMStoreFloat3(&previousBendDirs[bendCacheKey], bendDir);

    // 무릎이 꺾일 방향을 하나만 계산하기
    const float sinA = std::sqrt(std::max(0.0f, 1.0f - cosA * cosA));
    XMVECTOR newMidDir = XMVector3Normalize(XMVectorAdd(
        XMVectorScale(targetDir, cosA),
        XMVectorScale(bendDir, sinA)
    ));
    XMVECTOR newMidPos = XMVectorAdd(rootPos, XMVectorScale(newMidDir, a));

    // 중간 무릎 위치를 로그로 확인하기
    static uint32_t ikDebugLogCounter = 0;
    if ((++ikDebugLogCounter % 30) == 0)
    {
        XMFLOAT3 debugTargetDir;
        XMFLOAT3 debugPreIKMidDir;
        XMFLOAT3 debugBendDir;
        XMFLOAT3 debugCurrentMidPos;
        XMFLOAT3 debugNewMidPos;
        XMStoreFloat3(&debugTargetDir, targetDir);
        XMStoreFloat3(&debugPreIKMidDir, preIKMidDir);
        XMStoreFloat3(&debugBendDir, bendDir);
        XMStoreFloat3(&debugCurrentMidPos, midPos);
        XMStoreFloat3(&debugNewMidPos, newMidPos);
        //DEBUG_LOG_FMT(
        //    "[IKProcessor] targetDir=({:.3f},{:.3f},{:.3f}) preIKMidDir=({:.3f},{:.3f},{:.3f}) bendDir=({:.3f},{:.3f},{:.3f}) bendContinuity={:.3f} angleA={:.3f}\n",
        //    debugTargetDir.x, debugTargetDir.y, debugTargetDir.z, debugPreIKMidDir.x, debugPreIKMidDir.y,
        //    debugPreIKMidDir.z, debugBendDir.x, debugBendDir.y, debugBendDir.z,
        //    bendContinuity, angleA
        //);
        //DEBUG_LOG_FMT(
        //    "[IKProcessor] currentMid=({:.3f},{:.3f},{:.3f}) newMidPos=({:.3f},{:.3f},{:.3f})\n",
        //    debugCurrentMidPos.x, debugCurrentMidPos.y, debugCurrentMidPos.z, debugNewMidPos.x,
        //    debugNewMidPos.y, debugNewMidPos.z
        //);
    }
    XMVECTOR oldMidDir = currentMidDir;
    XMVECTOR rootRotQuat = GetRotationBetweenVectors(oldMidDir, newMidDir);
    XMMATRIX newRootWorld = rootWorld * XMMatrixRotationQuaternion(rootRotQuat);
    newRootWorld.r[3] = rootPos;

    // 아래다리 행렬 다시 계산
    XMVECTOR oldEndDir = XMVector3Normalize(XMVectorSubtract(endPos, midPos));
    XMVECTOR newEndDir = XMVector3Normalize(XMVectorSubtract(targetPos, newMidPos));
    XMVECTOR midRotQuat = GetRotationBetweenVectors(oldEndDir, newEndDir);
    XMMATRIX newMidWorld = midWorld * XMMatrixRotationQuaternion(midRotQuat);
    newMidWorld.r[3] = newMidPos;

    // weight에 따라 최종 행렬을 보간해서 저장하기
    float w = target.weight;
    XMStoreFloat4x4(&globalMatrices[target.rootBoneIndex], MatrixLerp(rootWorld, newRootWorld, w));
    XMStoreFloat4x4(&globalMatrices[target.midBoneIndex], MatrixLerp(midWorld, newMidWorld, w));

    // 발 끝 위치 업데이트
    XMMATRIX finalEnd = endWorld;
    finalEnd.r[3] = XMVectorLerp(endPos, targetPos, w);
    XMStoreFloat4x4(&globalMatrices[target.boneIndex], finalEnd);
}

