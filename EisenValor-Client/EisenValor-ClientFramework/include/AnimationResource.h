#pragma once
#include "IResource.h"
#include <AnimationData.h>
#include <vector>
#include <memory>

class AnimationResource final : public ResourceBase<AnimationResource>
{
public:
	static constexpr const char* GetStaticResourceName() { return "AnimationResource"; }

	AnimationResource() = default;
	~AnimationResource() override = default;

	// MetaData Getters
	uint64_t GetNameHash() const { return m_nameHash; }
	float	 GetDuration() const { return m_duration; }
	float	 GetFrameRate() const { return m_frameRate; }
	uint32_t GetTotalFrames() const { return m_totalFrames; }
	const EvAsset::Guid& GetSkeletonGuid() const { return m_skeletonGuid; }

	void SetData(const EvAsset::AnimationData& data);

	// 트랙 데이터 접근
	const std::vector<EvAsset::AnimationTrack>& GetTracks() const { return m_tracks; }

	// 애니메이션과 메시의 guid가 일치하는지 확인
	bool IsCompatible(std::shared_ptr<class SkinnedMeshResource> mesh) const;

private:
	uint64_t m_nameHash = 0;
	float	 m_duration = 0.0f;
	float	 m_frameRate = 30.0f;
	uint32_t m_totalFrames = 0;
	EvAsset::Guid m_skeletonGuid{};

	std::vector<EvAsset::AnimationTrack> m_tracks;
};
