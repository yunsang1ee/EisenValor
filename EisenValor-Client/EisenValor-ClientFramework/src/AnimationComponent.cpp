#include "stdafxClientFramework.h"
#include "AnimationComponent.h"
#include "GameObject.h"
#include "GameObject.inl"
#include "SkinnedMeshComponent.h"
#include "DxMath.h"
#include <cmath>
#include <algorithm>
#include <functional>

using namespace DirectX;

void AnimationComponent::OnUpdate(float dt)
{
	if (!m_isPlaying || !m_currentAnimation) return;

	m_currentTime += dt;
	float duration = m_currentAnimation->GetDuration();

	if (m_currentTime >= duration)
	{
		if (m_isLooping)
		{
			m_currentTime = std::fmod(m_currentTime, duration);
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

void AnimationComponent::Play(uint8_t key, bool loop)
{
	auto it = m_animations.find(key);
	if (it != m_animations.end())
	{
		Play(it->second, loop);
	}
	else
	{
		DEBUG_LOG_FMT("[AnimationComponent] Cannot find animation for key: {}\n", key);
	}
}

void AnimationComponent::Play(std::shared_ptr<AnimationResource> animation, bool loop)
{
	m_currentAnimation = animation;
	m_isLooping = loop;
	m_currentTime = 0.0f;
	m_isPlaying = true;
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

void AnimationComponent::UpdateBoneMatrices()
{
	if (!m_currentAnimation) return;

	auto* skinnedMesh = GetGameObject()->GetComponent<SkinnedMeshComponent>();
	if (!skinnedMesh || !skinnedMesh->IsValid()) return;

	auto meshRes = skinnedMesh->GetSkinnedMeshResource();
	const auto& bones = meshRes->GetBones();
	const size_t boneCount = bones.size();

	if (m_localMatrices.size() != boneCount)
	{
		m_localMatrices.resize(boneCount);
		m_globalMatrices.resize(boneCount);
		m_finalPalette.resize(boneCount);

		// [DEBUG] 매칭 확인
		const auto& tracks = m_currentAnimation->GetTracks();
		size_t matchCount = 0;
		for (const auto& track : tracks) {
			for (size_t i = 0; i < boneCount; ++i) {
				if (track.BoneNameHash == bones[i].nameHash) {
					matchCount++;
					break;
				}
			}
		}
		DEBUG_LOG_FMT("[Animation] Summary - Bones: {}, Tracks: {}, Matched: {}\n", boneCount, tracks.size(), matchCount);
	}

	float frameRate = m_currentAnimation->GetFrameRate();
	uint32_t totalFrames = m_currentAnimation->GetTotalFrames();
	float framePos = m_currentTime * frameRate;
	uint32_t frameIdx0 = static_cast<uint32_t>(std::floor(framePos)) % totalFrames;
	uint32_t frameIdx1 = (frameIdx0 + 1) % totalFrames;
	float alpha = framePos - std::floor(framePos);

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
				// Position 보정 (Unity -> DX)
				if (track.Flags & EvAsset::HasPos)
				{
					XMVECTOR p;
					if (track.Flags & EvAsset::IsConstPos)
					{
						p = XMVectorSet(track.Positions[0], track.Positions[1], track.Positions[2], 1.0f);
					}
					else
					{
						XMVECTOR p0 = XMVectorSet(track.Positions[frameIdx0 * 3 + 0], track.Positions[frameIdx0 * 3 + 1], track.Positions[frameIdx0 * 3 + 2], 1.0f);
						XMVECTOR p1 = XMVectorSet(track.Positions[frameIdx1 * 3 + 0], track.Positions[frameIdx1 * 3 + 1], track.Positions[frameIdx1 * 3 + 2], 1.0f);
						p = XMVectorLerp(p0, p1, alpha);
					}
					// 유니티와 DX의 X축 반전 보정
					pos = XMVectorSet(-XMVectorGetX(p), XMVectorGetY(p), XMVectorGetZ(p), 1.0f);
				}

				// Rotation 보정 (Unity -> DX)
				if (track.Flags & EvAsset::HasRot)
				{
					XMVECTOR r;
					if (track.Flags & EvAsset::IsConstRot)
					{
						r = XMVectorSet(track.Rotations[0], track.Rotations[1], track.Rotations[2], track.Rotations[3]);
					}
					else
					{
						XMVECTOR r0 = XMVectorSet(track.Rotations[frameIdx0 * 4 + 0], track.Rotations[frameIdx0 * 4 + 1], track.Rotations[frameIdx0 * 4 + 2], track.Rotations[frameIdx0 * 4 + 3]);
						XMVECTOR r1 = XMVectorSet(track.Rotations[frameIdx1 * 4 + 0], track.Rotations[frameIdx1 * 4 + 1], track.Rotations[frameIdx1 * 4 + 2], track.Rotations[frameIdx1 * 4 + 3]);
						r = XMQuaternionSlerp(r0, r1, alpha);
					}
					// 유니티 Quaternion 성분 보정 (Y, Z 반전)
					rot = XMVectorSet(XMVectorGetX(r), -XMVectorGetY(r), -XMVectorGetZ(r), XMVectorGetW(r));
				}

				if (track.Flags & EvAsset::HasScale)
				{
					if (track.Flags & EvAsset::IsConstScale)
					{
						scale = XMVectorSet(track.Scales[0], track.Scales[1], track.Scales[2], 0.0f);
					}
					else
					{
						XMVECTOR s0 = XMVectorSet(track.Scales[frameIdx0 * 3 + 0], track.Scales[frameIdx0 * 3 + 1], track.Scales[frameIdx0 * 3 + 2], 0.0f);
						XMVECTOR s1 = XMVectorSet(track.Scales[frameIdx1 * 3 + 0], track.Scales[frameIdx1 * 3 + 1], track.Scales[frameIdx1 * 3 + 2], 0.0f);
						scale = XMVectorLerp(s0, s1, alpha);
					}
				}
				break;
			}
		}

		XMMATRIX local = XMMatrixAffineTransformation(scale, XMVectorZero(), rot, pos);
		XMStoreFloat4x4(&m_localMatrices[i], local);
	}

	// 2. 계층 구조에 따른 전역 행렬 계산 (부모 순서 보장)
	std::vector<bool> computed(boneCount, false);
	const auto& offsetFloats = meshRes->GetOffsetMatrices();

	std::function<void(int32_t)> computeGlobal = [&](int32_t idx) {
		if (idx < 0 || computed[idx]) return;

		int32_t parentIdx = bones[idx].parentIndex;
		if (parentIdx != -1 && !computed[parentIdx]) {
			computeGlobal(parentIdx);
		}

		XMMATRIX local = XMLoadFloat4x4(&m_localMatrices[idx]);
		XMMATRIX global;
		if (parentIdx == -1) {
			global = local;
		} else {
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

	for (size_t i = 0; i < boneCount; ++i) {
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
