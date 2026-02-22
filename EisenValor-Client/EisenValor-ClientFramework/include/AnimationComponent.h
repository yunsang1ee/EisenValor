#pragma once
#include "IComponent.h"
#include "AnimationResource.h"
#include <DirectXMath.h>
#include <memory>
#include <vector>

class AnimationComponent : public ComponentBase<AnimationComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "AnimationComponent"; }

	AnimationComponent() = default;
	~AnimationComponent() override = default;

	void OnUpdate(float dt);

	void Play(std::shared_ptr<AnimationResource> animation, bool loop = true);
	void Stop();
	void Pause();
	void Resume();

	bool IsPlaying() const { return m_isPlaying; }

private:
	void UpdateBoneMatrices();

private:
	std::shared_ptr<AnimationResource> m_currentAnimation;
	float m_currentTime = 0.0f;
	bool m_isPlaying = false;
	bool m_isLooping = true;

	// 계산용 임시 버퍼들
	std::vector<DirectX::XMFLOAT4X4> m_localMatrices;
	std::vector<DirectX::XMFLOAT4X4> m_globalMatrices;
	std::vector<DirectX::XMFLOAT4X4> m_finalPalette;
};
