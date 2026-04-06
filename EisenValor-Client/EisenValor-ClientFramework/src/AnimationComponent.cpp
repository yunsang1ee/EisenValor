#include "stdafxClientFramework.h"
#include "AnimationComponent.h"
#include "GameObject.h"
#include "GameObject.inl"
#include "SkinnedMeshComponent.h"
#include "AssetFormat.h"
#include "DxMath.h"
#include <cmath>
#include <algorithm>
#include <functional>

using namespace DirectX;

void AnimationComponent::OnUpdate(float dt)
{
	if (!m_isPlaying || !m_currentAnimation)
		return;

	m_currentTime += dt;
	float duration = m_currentAnimation->GetDuration();

	if (m_currentTime >= duration)
	{
		if (m_isLooping)
		{
			m_currentTime = std::fmod(m_currentTime, duration);
			m_rootMotionFirstFrame = true;
		}
		else
		{
			m_currentTime = duration;
			m_isPlaying = false;
		}
	}

	UpdateBoneMatrices();
}

void AnimationComponent::AddAnimation(uint8_t key, std::shared_ptr<AnimationResource> animation)
{
	m_animations[key] = animation;
}

void AnimationComponent::Play(uint8_t key, bool loop, bool rootMotion)
{
	auto it = m_animations.find(key);
	if (it != m_animations.end())
	{
		m_currentKey = key;
		Play(it->second, loop, rootMotion);
	}
	else
	{
		DEBUG_LOG_FMT("[AnimationComponent] Cannot find animation for key: {}\n", key);
	}
}

void AnimationComponent::Play(std::shared_ptr<AnimationResource> animation, bool loop, bool rootMotion)
{
	m_currentAnimation = animation;
	m_isLooping = loop;
	m_enableRootMotion = rootMotion;
	m_currentTime = 0.0f;
	m_isPlaying = true;

	// 루트모션 초기화
	m_lastRootPos = { 0.0f, 0.0f, 0.0f };
	m_rootMotionFirstFrame = true;
}

void AnimationComponent::Stop()
{
	m_isPlaying = false;
	m_currentTime = 0.0f;
}

void AnimationComponent::Pause()
{
	m_isPlaying = false;
}

void AnimationComponent::Resume()
{
	if (m_currentAnimation)
		m_isPlaying = true;
}

bool AnimationComponent::GetBoneIndexByName(const std::string& boneName, uint32_t& outIndex) const
{
	auto* skinnedMesh = GetGameObject()->GetComponent<SkinnedMeshComponent>();
	if (!skinnedMesh || !skinnedMesh->IsValid())
		return false;

	auto meshRes = skinnedMesh->GetSkinnedMeshResource();
	if (!meshRes)
		return false;

	uint64_t targetHash = EvAsset::HashString(boneName);
	const auto& bones = meshRes->GetBones();
	for (size_t i = 0; i < bones.size(); ++i)
	{
		if (bones[i].nameHash == targetHash)
		{
			outIndex = static_cast<uint32_t>(i);
			return true;
		}
	}
	return false;
}

// 뼈의 인덱스를 받아 해당 뼈의 모델 행렬을 반환 (소켓 시스템용)
bool AnimationComponent::GetSocketMatrix(uint32_t boneIndex, DirectX::XMMATRIX& outMatrix) const
{
	if (boneIndex >= m_globalMatrices.size())
		return false;

	outMatrix = DirectX::XMLoadFloat4x4(&m_globalMatrices[boneIndex]);
	return true;
}

void AnimationComponent::UpdateBoneMatrices()
{
	if (!m_currentAnimation)
	{
		return;
	}

	auto* myGameObject = GetGameObject();
	auto* skinnedMesh = myGameObject->GetComponent<SkinnedMeshComponent>();
	if (!skinnedMesh || !skinnedMesh->IsValid())
	{
		return;
	}

	auto		 meshRes = skinnedMesh->GetSkinnedMeshResource();
	const auto&	 bones = meshRes->GetBones();
	const size_t boneCount = bones.size();

	if (m_localMatrices.size() != boneCount)
	{
		m_localMatrices.resize(boneCount);
		m_globalMatrices.resize(boneCount);
		m_finalPalette.resize(boneCount);

		// [DEBUG] 매칭 확인
		const auto& tracks = m_currentAnimation->GetTracks();
		size_t		matchCount = 0;
		for (const auto& track : tracks)
		{
			for (size_t i = 0; i < boneCount; ++i)
			{
				if (track.BoneNameHash == bones[i].nameHash)
				{
					matchCount++;
					break;
				}
			}
		}
		DEBUG_LOG_FMT(
			"[Animation] Summary - Bones: {}, Tracks: {}, Matched: {}\n", boneCount, tracks.size(), matchCount
		);
	}

	float	 frameRate = m_currentAnimation->GetFrameRate();
	uint32_t totalFrames = m_currentAnimation->GetTotalFrames();
	float	 framePos = m_currentTime * frameRate;
	uint32_t frameIdx0 = static_cast<uint32_t>(std::floor(framePos)) % totalFrames;
	uint32_t frameIdx1 = (frameIdx0 + 1) % totalFrames;
	float	 alpha = framePos - std::floor(framePos);

	const auto& tracks = m_currentAnimation->GetTracks();

	// 1. 모든 본의 로컬 행렬 계산
	for (size_t i = 0; i < boneCount; ++i)
	{
		XMVECTOR pos = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(bones[i].restPos));
		XMVECTOR rot = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(bones[i].restRot));
		XMVECTOR scale = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(bones[i].restScale));

		for (const auto& track : tracks)
		{
			if (track.BoneNameHash == bones[i].nameHash)
			{
				// Position
				if (track.Flags & EvAsset::HasPos)
				{
					XMVECTOR p;
					if (track.Flags & EvAsset::IsConstPos)
					{
						p = XMVectorSet(track.Positions[0], track.Positions[1], track.Positions[2], 1.0f);
					}
					else
					{
						XMVECTOR p0 = XMVectorSet(
							track.Positions[frameIdx0 * 3 + 0], track.Positions[frameIdx0 * 3 + 1],
							track.Positions[frameIdx0 * 3 + 2], 1.0f
						);
						XMVECTOR p1 = XMVectorSet(
							track.Positions[frameIdx1 * 3 + 0], track.Positions[frameIdx1 * 3 + 1],
							track.Positions[frameIdx1 * 3 + 2], 1.0f
						);
						p = XMVectorLerp(p0, p1, alpha);
					}
					//// 유니티와 DX의 X축 반전 보정
					//pos = XMVectorSet(-XMVectorGetX(p), XMVectorGetY(p), XMVectorGetZ(p), 1.0f);
					pos = p;
				}

				// Rotation
				if (track.Flags & EvAsset::HasRot)
				{
					XMVECTOR r;
					if (track.Flags & EvAsset::IsConstRot)
					{
						r = XMVectorSet(track.Rotations[0], track.Rotations[1], track.Rotations[2], track.Rotations[3]);
					}
					else
					{
						XMVECTOR r0 = XMVectorSet(
							track.Rotations[frameIdx0 * 4 + 0], track.Rotations[frameIdx0 * 4 + 1],
							track.Rotations[frameIdx0 * 4 + 2], track.Rotations[frameIdx0 * 4 + 3]
						);
						XMVECTOR r1 = XMVectorSet(
							track.Rotations[frameIdx1 * 4 + 0], track.Rotations[frameIdx1 * 4 + 1],
							track.Rotations[frameIdx1 * 4 + 2], track.Rotations[frameIdx1 * 4 + 3]
						);
						r = XMQuaternionSlerp(r0, r1, alpha);
					}
					rot = r;
				}

				if (track.Flags & EvAsset::HasScale)
				{
					if (track.Flags & EvAsset::IsConstScale)
					{
						scale = XMVectorSet(track.Scales[0], track.Scales[1], track.Scales[2], 0.0f);
					}
					else
					{
						XMVECTOR s0 = XMVectorSet(
							track.Scales[frameIdx0 * 3 + 0], track.Scales[frameIdx0 * 3 + 1],
							track.Scales[frameIdx0 * 3 + 2], 0.0f
						);
						XMVECTOR s1 = XMVectorSet(
							track.Scales[frameIdx1 * 3 + 0], track.Scales[frameIdx1 * 3 + 1],
							track.Scales[frameIdx1 * 3 + 2], 0.0f
						);
						scale = XMVectorLerp(s0, s1, alpha);
					}
				}
				break;
			}
		}

		// Root Motion 처리
		if (i == 0 && m_enableRootMotion)
		{
			XMFLOAT3 currentRootPos;
			XMStoreFloat3(&currentRootPos, pos);


			if (m_rootMotionFirstFrame)
			{
				// 첫번째 프레임: 기준점만 잡고 이동은 하지 않음
				m_lastRootPos = currentRootPos;
				m_rootMotionFirstFrame = false;
			}
			else
			{
				// Delta 계산 (현재 위치 - 이전 위치)
				XMVECTOR deltaVec = XMVectorSubtract(pos, XMLoadFloat3(&m_lastRootPos));

				// 현재 회전
				auto&	 transform = myGameObject->GetTransform();
				XMFLOAT4 worldRotQ = transform.GetWorldRotationQuaternion();
				
				XMFLOAT3 delta;
				XMStoreFloat3(&delta, deltaVec);
				delta.z = -delta.z; // Z축 반전

				XMVECTOR DeltaVec = XMVectorSet(delta.x, delta.y, delta.z, 0.0f);

				// 캐릭터 회전
				XMVECTOR rotQuat = XMLoadFloat4(&worldRotQ);
				XMVECTOR rotatedDelta = XMVector3Rotate(DeltaVec, rotQuat);

				// 델타 누적
				XMVECTOR currentAccDelta = XMLoadFloat3(&m_accumulatedRootDelta);
				XMVECTOR nextAccDelta = XMVectorAdd(currentAccDelta, rotatedDelta);
				XMStoreFloat3(&m_accumulatedRootDelta, nextAccDelta);

				// [DEBUG] 누적 이동량 확인 로그
				XMFLOAT3 acc;
				XMStoreFloat3(&acc, nextAccDelta);
				DEBUG_LOG_FMT("[RootMotion] Accumulated Delta: ({:.3f}, {:.3f}, {:.3f})\n", acc.x, acc.y, acc.z);

				// 기준점 업데이트
				m_lastRootPos = currentRootPos;
			}

			// 메시 위치 초기화
			pos = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		}

		XMMATRIX local = XMMatrixAffineTransformation(scale, XMVectorZero(), rot, pos);
		XMStoreFloat4x4(&m_localMatrices[i], local);
	}

	// 2. 계층 구조에 따른 전역 행렬 계산 (부모 순서 보장)
	std::vector<bool> computed(boneCount, false);
	const auto&		  offsetFloats = meshRes->GetOffsetMatrices();

	std::function<void(int32_t)> computeGlobal = [&](int32_t idx)
	{
		if (idx < 0 || computed[idx])
			return;

		int32_t parentIdx = bones[idx].parentIndex;
		if (parentIdx != -1 && !computed[parentIdx])
		{
			computeGlobal(parentIdx);
		}

		XMMATRIX local = XMLoadFloat4x4(&m_localMatrices[idx]);
		XMMATRIX global;
		if (parentIdx == -1)
		{
			global = local;
		}
		else
		{
			XMMATRIX parentGlobal = XMLoadFloat4x4(&m_globalMatrices[parentIdx]);
			global = XMMatrixMultiply(local, parentGlobal);
		}
		XMStoreFloat4x4(&m_globalMatrices[idx], global);

		// 3. 최종 팔레트 계산 (Offset * Global)
		XMMATRIX offset = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&offsetFloats[idx * 16]));
		XMMATRIX final = XMMatrixMultiply(offset, global);
		XMStoreFloat4x4(&m_finalPalette[idx], XMMatrixTranspose(final));

		computed[idx] = true;
	};

	for (size_t i = 0; i < boneCount; ++i)
	{
		computeGlobal((int32_t)i);
	}

	skinnedMesh->SetFinalMatrices(m_finalPalette);
}

bool AnimationComponent::IsAnimationEnd() const
{
	if (!m_currentAnimation || m_isLooping) 
	{
		return false;
	}
	return m_currentTime >= m_currentAnimation->GetDuration();
}
