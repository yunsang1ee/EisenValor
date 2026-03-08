#pragma once
#include <string>

class AnimationComponent;

namespace AnimationLoader
{
	// 캐릭터 이름에 맞는 애니메이션 리소스들을 로드하여 해당 AnimationComponent에 등록
	void AnimationApply(AnimationComponent* anim, const std::string& characterName);
}
