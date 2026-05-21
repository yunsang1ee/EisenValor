#include "stdafxClient.h"
#include "FootIKComponent.h"

#include "AnimationComponent.h"
#include "GameObject.h"
#include "GameObject.inl"

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
		DEBUG_LOG_FMT(
			"[FootIK] leftFootY={:.3f}, leftIkFootY={:.3f}, rightFootY={:.3f}, rightIkFootY={:.3f}\n",
			DirectX::XMVectorGetY(leftFootMatrix.r[3]), DirectX::XMVectorGetY(leftTargetMatrix.r[3]),
			DirectX::XMVectorGetY(rightFootMatrix.r[3]), DirectX::XMVectorGetY(rightTargetMatrix.r[3])
		);
	}

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
