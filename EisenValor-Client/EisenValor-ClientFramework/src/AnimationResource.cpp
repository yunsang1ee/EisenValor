#include "stdafxClientFramework.h"
#include "AnimationResource.h"
#include "SkinnedMeshResource.h"

void AnimationResource::SetData(const EvAsset::AnimationData& data)
{
	m_nameHash = data.nameHash;
	m_duration = data.duration;
	m_frameRate = data.frameRate;
	m_totalFrames = data.totalFrames;
	m_skeletonGuid = data.skeletonGuid;

	// 트랙 데이터 이동
	// AnimationData는 로딩 후 바로 파기되는 임시 객체
	m_tracks = std::move(const_cast<EvAsset::AnimationData&>(data).tracks);
}

bool AnimationResource::IsCompatible(std::shared_ptr<SkinnedMeshResource> mesh) const
{
	if (!mesh)
	{
		return false;
	}

	if (m_skeletonGuid != mesh->GetGuid())
	{
		return false;
	}

	const uint32_t boneCount = static_cast<uint32_t>(mesh->GetBones().size());
	/*
	for (const auto& track : m_tracks)
	{
		if (track.BoneIndex >= boneCount)
		{
			DEBUG_LOG_FMT("[AnimationResource] Incompatible: BoneIndex {} is out of range (Mesh bones: {})\n", track.BoneIndex, boneCount);
			return false;
		}
	}
	*/

	return true;
}
