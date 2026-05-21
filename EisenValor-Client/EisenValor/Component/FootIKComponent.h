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

	bool CacheBones(AnimationComponent& animation);
	bool IsValidLegCache(const LegBoneCache& cache) const;
	IKTarget BuildLegIKTarget(const LegBoneCache& cache, DirectX::FXMVECTOR targetPos, float weight) const;

private:
	bool m_ikEnabled = true;
	bool m_bonesCached = false;

	LegBoneCache m_leftLeg;
	LegBoneCache m_rightLeg;
	uint32_t	 m_ikFootRoot = kInvalidBoneIndex;

	float m_leftWeight = 0.0f;
	float m_rightWeight = 0.0f;
	float m_pelvisOffsetY = 0.0f;

	float m_footSoleOffset = 0.0f;
	float m_maxPelvisDrop = 0.25f;
};
