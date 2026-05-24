#include "stdafxClient.h"
#include "FootIKComponent.h"

#include "AnimationComponent.h"
#include "GameObject.h"
#include "GameObject.inl"
#include "MeshComponent.h"
#include "MeshResource.h"
#include "Scene.h"
#include "Transform.h"

#include <cmath>
#include <unordered_set>

namespace
{
bool TryGetBone(AnimationComponent& animation, const char* boneName, uint32_t& outIndex)
{
	if (animation.GetBoneIndexByName(boneName, outIndex))
	{
		return true;
	}

	DEBUG_LOG_FMT("[FootIK] Missing bone '{}'\n", boneName);
	outIndex = UINT32_MAX;
	return false;
}

bool TryGetBoneMatrix(AnimationComponent& animation, uint32_t boneIndex, DirectX::XMMATRIX& outMatrix)
{
	return boneIndex != UINT32_MAX && animation.GetSocketMatrix(boneIndex, outMatrix);
}

DirectX::XMVECTOR TransformBonePositionToWorld(DirectX::FXMVECTOR bonePosition, const Transform& ownerTransform)
{
	const auto ownerWorldMatrix = ownerTransform.GetWorldMatrix();
	return DirectX::XMVector3TransformCoord(bonePosition, DirectX::XMLoadFloat4x4(&ownerWorldMatrix));
}

}

} // namespace

void FootIKComponent::OnStart()
{
	if (auto* animation = GetGameObject()->GetComponent<AnimationComponent>())
	{
		m_bonesCached = CacheBones(*animation);
		m_ikEnabled = m_bonesCached;
	}
}

void FootIKComponent::OnLateUpdate(float)
{
	if (!m_ikEnabled)
	{
		return;
	}

	auto* owner = GetGameObject();
	auto* animation = owner ? owner->GetComponent<AnimationComponent>() : nullptr;
	if (!animation)
	{
		return;
	}

	if (!m_bonesCached)
	{
		m_bonesCached = CacheBones(*animation);
		m_ikEnabled = m_bonesCached;
	}

	if (!m_bonesCached)
	{
		return;
	}

	DirectX::XMMATRIX leftFootMatrix;
	DirectX::XMMATRIX leftTargetMatrix;
	DirectX::XMMATRIX rightFootMatrix;
	DirectX::XMMATRIX rightTargetMatrix;
	if (!TryGetBoneMatrix(*animation, m_leftLeg.foot, leftFootMatrix) ||
		!TryGetBoneMatrix(*animation, m_leftLeg.ikFoot, leftTargetMatrix) ||
		!TryGetBoneMatrix(*animation, m_rightLeg.foot, rightFootMatrix) ||
		!TryGetBoneMatrix(*animation, m_rightLeg.ikFoot, rightTargetMatrix))
	{
		return;
	}

	static std::unordered_set<const FootIKComponent*> loggedComponents;
	if (loggedComponents.insert(this).second)
	{
		auto&		ownerTransform = owner->GetTransform();
		const auto  ownerWorldPosition = ownerTransform.GetWorldPosition();
		const auto  leftFootWorld = TransformBonePositionToWorld(leftFootMatrix.r[3], ownerTransform);
		const auto  leftTargetWorld = TransformBonePositionToWorld(leftTargetMatrix.r[3], ownerTransform);
		const auto  rightFootWorld = TransformBonePositionToWorld(rightFootMatrix.r[3], ownerTransform);
		const auto  rightTargetWorld = TransformBonePositionToWorld(rightTargetMatrix.r[3], ownerTransform);
		DirectX::XMFLOAT3 leftFootWorldPosition;
		DirectX::XMFLOAT3 rightFootWorldPosition;
		DirectX::XMStoreFloat3(&leftFootWorldPosition, leftFootWorld);
		DirectX::XMStoreFloat3(&rightFootWorldPosition, rightFootWorld);
		const DirectX::XMFLOAT3 footCenter = {
			(leftFootWorldPosition.x + rightFootWorldPosition.x) * 0.5f,
			(leftFootWorldPosition.y + rightFootWorldPosition.y) * 0.5f,
			(leftFootWorldPosition.z + rightFootWorldPosition.z) * 0.5f
		};

		DEBUG_LOG_FMT(
			"[FootIK] modelY leftFoot={:.3f}, leftIkFoot={:.3f}, rightFoot={:.3f}, rightIkFoot={:.3f}\n",
			DirectX::XMVectorGetY(leftFootMatrix.r[3]), DirectX::XMVectorGetY(leftTargetMatrix.r[3]),
			DirectX::XMVectorGetY(rightFootMatrix.r[3]), DirectX::XMVectorGetY(rightTargetMatrix.r[3])
		);
		DEBUG_LOG_FMT(
			"[FootIK] worldY player={:.3f}, leftFoot={:.3f}, leftIkFoot={:.3f}, rightFoot={:.3f}, rightIkFoot={:.3f}\n",
			ownerWorldPosition.y, DirectX::XMVectorGetY(leftFootWorld), DirectX::XMVectorGetY(leftTargetWorld),
			DirectX::XMVectorGetY(rightFootWorld), DirectX::XMVectorGetY(rightTargetWorld)
		);

		if (auto* scene = owner->GetScene())
		{
			if (auto* meshStorage = scene->GetStorage<MeshComponent>())
			{
				DEBUG_LOG_FMT("[FootIK] scene mesh count={}\n", meshStorage->GetList().size());

				constexpr float nearbyRadius = 20.0f;
				constexpr float nearbyRadiusSq = nearbyRadius * nearbyRadius;
				size_t nearbyCandidateCount = 0;
				size_t loggedCandidateCount = 0;
				size_t loggedMeshCount = 0;
				for (const auto& meshComp : meshStorage->GetList())
				{
					if (!meshComp.IsValid())
					{
						continue;
					}

					auto* meshObj = meshComp.GetGameObject();
					auto* meshRes = meshComp.GetMeshResource();
					if (!meshObj || !meshRes)
					{
						continue;
					}

					const auto worldPos = meshObj->GetTransform().GetWorldPosition();
					if (loggedMeshCount < 12)
					{
						DEBUG_LOG_FMT(
							"[FootIK] scene mesh obj='{}' guid={} world=({:.3f}, {:.3f}, {:.3f})\n",
							meshObj->GetName().c_str(), meshRes->GetGuid(), worldPos.x, worldPos.y, worldPos.z
						);
						++loggedMeshCount;
					}

					const float distanceSqXZ = DistanceSqXZ(worldPos, footCenter);
					if (distanceSqXZ <= nearbyRadiusSq)
					{
						++nearbyCandidateCount;

						if (loggedCandidateCount < 8)
						{
							DEBUG_LOG_FMT(
								"[FootIK] nearby mesh candidate obj='{}' guid={} world=({:.3f}, {:.3f}, {:.3f}) distXZ={:.3f}\n",
								meshObj->GetName().c_str(), meshRes->GetGuid(), worldPos.x, worldPos.y, worldPos.z,
								std::sqrt(distanceSqXZ)
							);
							++loggedCandidateCount;
						}
					}

				}

				DEBUG_LOG_FMT(
					"[FootIK] nearby mesh candidate count={} radius={:.1f} footCenter=({:.3f}, {:.3f}, {:.3f})\n",
					nearbyCandidateCount, nearbyRadius, footCenter.x, footCenter.y, footCenter.z
				);
			}
		}
	}

	auto&	   ownerTransform = owner->GetTransform();
	const auto leftFootWorld = TransformBonePositionToWorld(leftFootMatrix.r[3], ownerTransform);
	const auto rightFootWorld = TransformBonePositionToWorld(rightFootMatrix.r[3], ownerTransform);
	DirectX::XMFLOAT3 leftFootWorldPosition;
	DirectX::XMFLOAT3 rightFootWorldPosition;
	DirectX::XMStoreFloat3(&leftFootWorldPosition, leftFootWorld);
	DirectX::XMStoreFloat3(&rightFootWorldPosition, rightFootWorld);

	GroundHit leftGroundHit;
	GroundHit rightGroundHit;
	const bool leftHit = TrySampleVisualGround(leftFootWorldPosition, 0.5f, 1.0f, leftGroundHit);
	const bool rightHit = TrySampleVisualGround(rightFootWorldPosition, 0.5f, 1.0f, rightGroundHit);
	m_leftWeight = leftHit ? m_leftWeight : 0.0f;
	m_rightWeight = rightHit ? m_rightWeight : 0.0f;

	animation->SetIKTarget(
		IK_TYPE::LEFT_LEG, BuildLegIKTarget(m_leftLeg, leftTargetMatrix.r[3], m_leftWeight)
	);
	animation->SetIKTarget(
		IK_TYPE::RIGHT_LEG, BuildLegIKTarget(m_rightLeg, rightTargetMatrix.r[3], m_rightWeight)
	);
}

bool FootIKComponent::CacheBones(AnimationComponent& animation)
{
	bool success = true;

	success &= TryGetBone(animation, "thigh_l", m_leftLeg.thigh);
	success &= TryGetBone(animation, "calf_l", m_leftLeg.calf);
	success &= TryGetBone(animation, "foot_l", m_leftLeg.foot);
	success &= TryGetBone(animation, "ik_foot_l", m_leftLeg.ikFoot);

	success &= TryGetBone(animation, "thigh_r", m_rightLeg.thigh);
	success &= TryGetBone(animation, "calf_r", m_rightLeg.calf);
	success &= TryGetBone(animation, "foot_r", m_rightLeg.foot);
	success &= TryGetBone(animation, "ik_foot_r", m_rightLeg.ikFoot);

	success &= TryGetBone(animation, "ik_foot_root", m_ikFootRoot);

	if (!success)
	{
		DEBUG_LOG_FMT("[FootIK] Bone cache failed. IK targets will stay disabled.\n");
	}

	return success && IsValidLegCache(m_leftLeg) && IsValidLegCache(m_rightLeg);
}

bool FootIKComponent::IsValidLegCache(const LegBoneCache& cache) const
{
	return cache.thigh != kInvalidBoneIndex && cache.calf != kInvalidBoneIndex && cache.foot != kInvalidBoneIndex &&
		   cache.ikFoot != kInvalidBoneIndex;
}

IKTarget FootIKComponent::BuildLegIKTarget(
	const LegBoneCache& cache, DirectX::FXMVECTOR targetPos, float weight
) const
{
	IKTarget target;
	target.rootBoneIndex = cache.thigh;
	target.midBoneIndex = cache.calf;
	target.boneIndex = cache.foot;
	target.targetPos = targetPos;
	target.poleVector = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	target.weight = weight;
	target.active = true;
	return target;
}

bool FootIKComponent::TrySampleVisualGround(
	const DirectX::XMFLOAT3&, float, float, GroundHit&
) const
{
	return false;
}
