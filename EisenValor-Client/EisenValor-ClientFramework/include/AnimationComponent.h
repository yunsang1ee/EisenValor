#pragma once
#include "IComponent.h"
#include "AnimationResource.h"
#include "IKProcessor.h"
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <optional>
#include <deque>
#include <array>
#include <functional>

// IK를 적용할 수 있는 부위
enum class IK_TYPE : uint8_t {
	RIGHT_ARM = 0,
	LEFT_ARM,
	RIGHT_LEG,
	LEFT_LEG,
	COUNT // 배열 크기용
};

class AnimationComponent : public ComponentBase<AnimationComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "AnimationComponent"; }
	static constexpr int		 kPriority = -100;

	AnimationComponent() = default;
	~AnimationComponent() override = default;

	void OnLateUpdate(float dt);

	// 애니메이션 등록 및 키 기반 재생
	void AddAnimation(uint8_t key, std::shared_ptr<AnimationResource> animation);
	void AddAnimationEvent(uint8_t key, float time, std::function<void()> callback);
	void Play(uint8_t key, bool loop = true, bool rootMotion = false);
	void PlayBlend(uint8_t key, float duration, bool loop = true, bool rootMotion = false);
	void Play(std::shared_ptr<AnimationResource> animation, bool loop = true, bool rootMotion = false);

	// Animation Queue System
	void PlayQueue(const std::vector<uint8_t>& keys, bool finalLoop = true, bool rootMotion = false);
	void SetNextAnimation(uint8_t nextKey, bool loop = true, bool rootMotion = false);

	void Stop();
	void Pause();
	void Resume();

	bool IsPlaying() const { return m_isPlaying; }
	bool IsAnimationEnd() const;

	uint8_t GetCurrentKey() const { return m_currentKey; }

	// IK 설정 함수 (Enum 사용)
	void SetIKTarget(IK_TYPE type, const IKTarget& target);
	void SetIKWeight(IK_TYPE type, float weight);
	void ClearIKTargets();
	void SetModelRootOffsetY(float offsetY) { m_modelRootOffsetY = offsetY; }

	// 루트 모션 이동량 가져오기
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
	bool GetPreIKSocketMatrix(uint32_t boneIndex, DirectX::XMMATRIX& outMatrix) const;


private:
	void UpdateBoneMatrices();
	void DispatchAnimationEvents(float previousTime, float currentTime, float duration, bool looped);

private:
	struct AnimationEvent
	{
		float time = 0.0f;
		std::function<void()> callback;
	};

	std::unordered_map<uint8_t, std::shared_ptr<AnimationResource>> m_animations;
	std::unordered_map<uint8_t, std::vector<AnimationEvent>> m_animationEvents;
	std::shared_ptr<AnimationResource> m_currentAnimation;
	uint8_t m_currentKey = 0;
	float m_currentTime = 0.0f;
	bool m_isPlaying = false;
	bool m_isLooping = true;

	std::shared_ptr<AnimationResource> m_blendFromAnimation;
	float m_blendFromTime = 0.0f;
	float m_blendTime = 0.0f;
	float m_blendDuration = 0.0f;
	bool m_isBlending = false;

	// 큐 시스템 데이터
	struct AnimationQueueInfo {
		uint8_t key;
		bool loop;
		bool rootMotion;
	};
	std::deque<AnimationQueueInfo> m_animationQueue;

	// IK 데이터
	IKProcessor m_ikProcessor;
	std::array<IKTarget, static_cast<size_t>(IK_TYPE::COUNT)> m_ikTargets;
	float m_modelRootOffsetY = 0.0f;

	// 루트모션
	bool m_enableRootMotion = false;
	bool m_rootMotionFirstFrame = true;
	DirectX::XMFLOAT3 m_lastRootPos = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_accumulatedRootDelta = { 0.0f, 0.0f, 0.0f };

	// 계산용 임시 버퍼들
	std::vector<DirectX::XMFLOAT4X4> m_localMatrices;
	std::vector<DirectX::XMFLOAT4X4> m_globalMatrices;
	std::vector<DirectX::XMFLOAT4X4> m_preIKGlobalMatrices;
	std::vector<DirectX::XMFLOAT4X4> m_finalPalette;
};
