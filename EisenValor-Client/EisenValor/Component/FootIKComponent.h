#pragma once
#include "IComponent.h"
#include "IKProcessor.h"
#include <cstdint>

class AnimationComponent;

class FootIKComponent : public ComponentBase<FootIKComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "FootIKComponent"; }
	static constexpr int		 kPriority = -101;

	void OnStart() override;
	void OnLateUpdate(float deltaTime);

	void SetEnabledForIK(bool enabled) { m_ikEnabled = enabled; }
	void SetFootSoleOffset(float offset) { m_footSoleOffset = offset; }
	void SetMaxPelvisDrop(float maxDrop) { m_maxPelvisDrop = maxDrop; }

private:
	static constexpr uint32_t kInvalidBoneIndex = UINT32_MAX;

	struct LegBoneCache
	{
		uint32_t thigh = kInvalidBoneIndex;
		uint32_t calf = kInvalidBoneIndex;
		uint32_t foot = kInvalidBoneIndex;
		uint32_t ikFoot = kInvalidBoneIndex;
	};

	struct GroundHit
	{
		DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT3 normal = {0.0f, 1.0f, 0.0f};
		float distance = 0.0f;
	};

	bool CacheBones(AnimationComponent& animation);
	bool IsValidLegCache(const LegBoneCache& cache) const;
	bool TrySampleVisualGround(
		const DirectX::XMFLOAT3& worldPosition, float maxUp, float maxDown, GroundHit& outHit
	) const;

private:
	bool m_ikEnabled = true;
	bool m_bonesCached = false;

	LegBoneCache m_leftLeg;
	LegBoneCache m_rightLeg;
	uint32_t	 m_ikFootRoot = kInvalidBoneIndex;

	float m_leftWeight = 0.0f;
	float m_rightWeight = 0.0f;
	float m_pelvisOffsetY = 0.0f;
	float m_prevLeftTargetGap = 0.0f;
	float m_prevRightTargetGap = 0.0f;
	bool  m_hasPreviousTargetGap = false;

	float m_footSoleOffset = 0.0f;
	float m_maxPelvisDrop = 0.8f;
};
