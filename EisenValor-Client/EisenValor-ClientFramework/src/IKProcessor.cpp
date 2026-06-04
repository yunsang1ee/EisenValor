#include "stdafxClientFramework.h"
#include "IKProcessor.h"
#include <cmath>
#include <algorithm>

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

void IKProcessor::SolveTwoBoneIK(std::vector<XMFLOAT4X4>& globalMatrices, const IKTarget& target)
{
    if (!target.active || target.weight <= 0.0f)
        return;

    // 각 본의 현재 월드 행렬
    XMMATRIX rootWorld = XMLoadFloat4x4(&globalMatrices[target.rootBoneIndex]);
    XMMATRIX midWorld = XMLoadFloat4x4(&globalMatrices[target.midBoneIndex]);
    XMMATRIX endWorld = XMLoadFloat4x4(&globalMatrices[target.boneIndex]);

    // 각 관절의 월드 위치
    XMVECTOR rootPos = rootWorld.r[3];
    XMVECTOR midPos = midWorld.r[3];
    XMVECTOR endPos = endWorld.r[3];
    XMVECTOR targetPos = target.targetPos;

    // 본의 길이 계산
    float a = XMVectorGetX(XMVector3Length(XMVectorSubtract(midPos, rootPos))); 
    float b = XMVectorGetX(XMVector3Length(XMVectorSubtract(endPos, midPos)));  
    float c = XMVectorGetX(XMVector3Length(XMVectorSubtract(targetPos, rootPos))); 

    // 거리 제약 (팔을 너무 펴거나 굽히지 않도록)
    c = std::clamp(c, std::abs(a - b) + 0.001f, a + b - 0.001f);

    // 코사인 법칙으로 팔꿈치 각도 구하기
    float cosC = (a * a + b * b - c * c) / (2.0f * a * b);
    float angleC = std::acos(std::clamp(cosC, -1.0f, 1.0f));

    // 어깨 각도 구하기
    float cosA = (a * a + c * c - b * b) / (2.0f * a * c);
    float angleA = std::acos(std::clamp(cosA, -1.0f, 1.0f));

    // 회전 Plane 결정 - 목표 방향과 팔꿈치(poleVector) 이용
    XMVECTOR targetDir = XMVector3Normalize(XMVectorSubtract(targetPos, rootPos));
    XMVECTOR currentMidDir = XMVector3Normalize(XMVectorSubtract(midPos, rootPos));
    XMVECTOR poleDir = XMVector3Normalize(target.poleVector);
    poleDir = XMVectorSubtract(poleDir, XMVectorScale(targetDir, XMVectorGetX(XMVector3Dot(poleDir, targetDir))));
    
    // 원래 애니메이션에서 팔꿈치 방향 받아와서 가까운 방향으로 poleDir 조정
    if (XMVectorGetX(XMVector3LengthSq(poleDir)) < 0.000001f)
    {
        poleDir = XMVectorSubtract(
            currentMidDir,
            XMVectorScale(targetDir, XMVectorGetX(XMVector3Dot(currentMidDir, targetDir)))
        );
    }
	// 그래도 거의 0이면, targetDir과 수직인 임의의 방향 선택
    if (XMVectorGetX(XMVector3LengthSq(poleDir)) < 0.000001f)
    {
        const XMVECTOR upAxis = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        const XMVECTOR rightAxis = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
        const XMVECTOR fallbackAxis =
            std::abs(XMVectorGetX(XMVector3Dot(targetDir, upAxis))) < 0.95f ? upAxis : rightAxis;
        poleDir = XMVectorSubtract(
            fallbackAxis,
            XMVectorScale(targetDir, XMVectorGetX(XMVector3Dot(fallbackAxis, targetDir)))
        );
    }
    poleDir = XMVector3Normalize(poleDir);
    XMVECTOR sideDir = XMVector3Normalize(XMVector3Cross(targetDir, poleDir));

    // 어깨에서 팔꿈치로 향하는 새로운 벡터 계산
    XMVECTOR midDirCandidateA = XMVector3Rotate(targetDir, XMQuaternionRotationAxis(sideDir, -angleA));
    XMVECTOR midDirCandidateB = XMVector3Rotate(targetDir, XMQuaternionRotationAxis(sideDir, angleA));
    XMVECTOR midPosCandidateA = XMVectorAdd(rootPos, XMVectorScale(midDirCandidateA, a));
    XMVECTOR midPosCandidateB = XMVectorAdd(rootPos, XMVectorScale(midDirCandidateB, a));
    float midDistanceA = XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(midPosCandidateA, midPos)));
    float midDistanceB = XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(midPosCandidateB, midPos)));
    XMVECTOR newMidDir = midDistanceA <= midDistanceB ? midDirCandidateA : midDirCandidateB;
    XMVECTOR newMidPos = midDistanceA <= midDistanceB ? midPosCandidateA : midPosCandidateB;

    // 어깨 행렬
    XMVECTOR oldMidDir = currentMidDir;
    XMVECTOR rootRotQuat = GetRotationBetweenVectors(oldMidDir, newMidDir);
    XMMATRIX newRootWorld = rootWorld * XMMatrixRotationQuaternion(rootRotQuat);
    newRootWorld.r[3] = rootPos;

    // 팔꿈치 행렬
    XMVECTOR oldEndDir = XMVector3Normalize(XMVectorSubtract(endPos, midPos));
    XMVECTOR newEndDir = XMVector3Normalize(XMVectorSubtract(targetPos, newMidPos));
    XMVECTOR midRotQuat = GetRotationBetweenVectors(oldEndDir, newEndDir);
    XMMATRIX newMidWorld = midWorld * XMMatrixRotationQuaternion(midRotQuat);
    newMidWorld.r[3] = newMidPos;

    // Weight에 따른 최종 보간 및 저장
    float w = target.weight;
    XMStoreFloat4x4(&globalMatrices[target.rootBoneIndex], MatrixLerp(rootWorld, newRootWorld, w));
    XMStoreFloat4x4(&globalMatrices[target.midBoneIndex], MatrixLerp(midWorld, newMidWorld, w));

    // 손목 위치 업데이트
    XMMATRIX finalEnd = endWorld;
    finalEnd.r[3] = XMVectorLerp(endPos, targetPos, w);
    XMStoreFloat4x4(&globalMatrices[target.boneIndex], finalEnd);
}

