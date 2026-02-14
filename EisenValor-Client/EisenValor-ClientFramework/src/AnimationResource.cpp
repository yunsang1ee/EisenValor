#include "stdafxClientFramework.h"
#include "AnimationResource.h"

void AnimationResource::SetData(const EvAsset::AnimationData& data)
{
	m_nameHash = data.meta.nameHash;
	m_duration = data.meta.duration;
	m_frameRate = data.meta.frameRate;
	m_totalFrames = data.meta.totalFrames;
	m_skeletonGuid = data.meta.skeletonGuid;

	// 트랙 데이터 이동
	// AnimationData는 로딩 후 바로 파기되는 임시 객체
	m_tracks = std::move(const_cast<EvAsset::AnimationData&>(data).tracks);
}
