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
	void Play(uint8_t key, bool loop = true, bool rootMotion = false);

	void Play(std::shared_ptr<AnimationResource> animation, bool loop = true, bool rootMotion = false);
	void Stop();
	void Pause();
	void Resume();

	bool IsPlaying() const { return m_isPlaying; }
	bool IsAnimationEnd() const;

	uint8_t GetCurrentKey() const { return m_currentKey; }

	// 루트 모션 이동량 가져오기 및 초기화
	DirectX::XMVECTOR ConsumeRootMotionDelta() 
	{ 
		DirectX::XMVECTOR delta = DirectX::XMLoadFloat3(&m_accumulatedRootDelta);
		m_accumulatedRootDelta = { 0.0f, 0.0f, 0.0f };
		return delta;
	}

	// Final Bone Palette
	const std::vector<DirectX::XMFLOAT4X4>& GetFinalPalette() const { return m_finalPalette; }
	
	// Socket System
    bool GetBoneIndexByName(const std::string& boneName, uint32_t& outIndex) const;
    bool GetSocketMatrix(uint32_t boneIndex, DirectX::XMMATRIX& outMatrix) const;


private:
	void UpdateBoneMatrices();

private:
	std::unordered_map<uint8_t, std::shared_ptr<AnimationResource>> m_animations;
	std::shared_ptr<AnimationResource> m_currentAnimation;
	uint8_t m_currentKey = 0;
	float m_currentTime = 0.0f;
	bool m_isPlaying = false;
	bool m_isLooping = true;

	// 루트모션
	bool m_enableRootMotion = false;
	bool m_rootMotionFirstFrame = true;
	DirectX::XMFLOAT3 m_lastRootPos = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_accumulatedRootDelta = { 0.0f, 0.0f, 0.0f };

	// 계산용 임시 버퍼들
	std::vector<DirectX::XMFLOAT4X4> m_localMatrices;
	std::vector<DirectX::XMFLOAT4X4> m_globalMatrices;
	std::vector<DirectX::XMFLOAT4X4> m_finalPalette;
};
