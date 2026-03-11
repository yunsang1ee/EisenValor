#pragma once
#include "IComponent.h"
#include "AnimationResource.h"
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <unordered_map>

class AnimationComponent : public ComponentBase<AnimationComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "AnimationComponent"; }

	AnimationComponent() = default;
	~AnimationComponent() override = default;

	void OnUpdate(float dt);

	// 애니메이션 등록 및 키 기반 재생 (uint8_t key 사용)
	void AddAnimation(uint8_t key, std::shared_ptr<AnimationResource> animation);
	void Play(uint8_t key, bool loop = true);

	void Play(std::shared_ptr<AnimationResource> animation, bool loop = true);
	void Stop();
	void Pause();
	void Resume();

	bool IsPlaying() const { return m_isPlaying; }

private:
	void UpdateBoneMatrices();

private:
	std::unordered_map<uint8_t, std::shared_ptr<AnimationResource>> m_animations;
	std::shared_ptr<AnimationResource> m_currentAnimation;
	float m_currentTime = 0.0f;
	bool m_isPlaying = false;
	bool m_isLooping = true;

	// 계산용 임시 버퍼들
	std::vector<DirectX::XMFLOAT4X4> m_localMatrices;
	std::vector<DirectX::XMFLOAT4X4> m_globalMatrices;
	std::vector<DirectX::XMFLOAT4X4> m_finalPalette;
};
