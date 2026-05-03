#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include <IComponent.h>
#include "StatePool.h"
#include "Util/GameConstants.h"

// 캐릭터의 상태 데이터를 보유하고 관리하는 컴포넌트
// 실제 행동 로직은 StatePool에서 가져옴
class FSMComponent : public ComponentBase<FSMComponent> {
public:
	static constexpr const char* GetStaticTypeName() { return "FSMComponent"; }

	enum class StateRequestType : uint8_t {
		Move,
		StopMove,
		AttackLight,
		AttackHeavy,
		AttackArea,
		AttackDisarm,
		CancelAttack,
		Stun,
		Die,
		IdleRecovery,
		ForcedServerCorrection
	};

	FSMComponent() = default;
	virtual ~FSMComponent() = default;

	// IComponent Lifecycle
	void OnUpdate(float deltaTime);

	// 상태 전이, 현재 상태 조회
	//서버
	void SetServerState(uint8_t serverState);
	uint8_t GetServerState() const { return m_serverState; }

	//클라
	bool RequestState(StateRequestType request, uint8_t requestedStateType = 0);
	void ChangeState(uint8_t nextStateType);
	uint8_t GetCurStateType() const { return m_curStateType; }

	// 타이머
	void  SetStateTimer(float time) { m_stateTimer = time; }
	float GetStateTimer() const { return m_stateTimer; }
	void  AddStateTimer(float dt) { m_stateTimer += dt; }

	// 공격 타입 기억 (사이클 유지용)
	void    SetCurAttackType(uint8_t type) { m_curAttackType = type; }
	uint8_t GetCurAttackType() const { return m_curAttackType; }

	// 공격 조준 방향 (FB_ENUMS::GENERAL_ATTACK_DIR_TYPE)
	void SetCurAttackDir(uint8_t dir)
	{
		//if (m_curAttackDir != dir)
		//{
		//	DEBUG_LOG_FMT("[FSM] Direction Changed: {} -> {}\n", m_curAttackDir, dir);
		//}
		m_curAttackDir = dir;
	}
	uint8_t GetCurAttackDir() const { return m_curAttackDir; }

	// 캐릭터 타입
	void    SetObjectType(uint8_t type) { m_objType = type; }
	uint8_t GetObjectType() const { return m_objType; }

	void SetMoveDirection(FB_ENUMS::MOVE_DIRECTION_TYPE dir) { m_moveDir = dir; }
	FB_ENUMS::MOVE_DIRECTION_TYPE GetMoveDirection() const { return m_moveDir; }

	// 락온 상태 설정/조회
	void SetLockOn(bool lockOn) { m_isLockOn = lockOn; }
	bool IsLockOn() const { return m_isLockOn; }

	// Stance 설정/조회 (FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL/BATTLE)
	void    SetStance(uint8_t stance);
	uint8_t GetStance() const { return m_stance; }

	// Observer Pattern(상태 ID가 인자)
	using StateChangeListener = std::function<void(uint8_t)>;
	void AddListener(StateChangeListener listener) { m_listeners.push_back(listener); }

	using StanceChangeListener = std::function<void(uint8_t)>;
	void AddStanceListener(StanceChangeListener listener) { m_stanceListeners.push_back(listener); }

private:
	bool ResolveStateRequest(StateRequestType request, uint8_t requestedStateType, uint8_t& outNextStateType) const;
	bool IsAttackSequenceState(uint8_t stateType) const;
	void ChangeState(uint8_t nextStateType, bool ignoreExitTime);

	// 캐릭터별 데이터
	uint8_t m_serverState = 0;   // 서버에서 보낸 상태 (GENERAL_STATE_TYPE)
	uint8_t m_curStateType = 0;  // 클라이언트 애니메이션 상태 (GENERAL/PLAYER_STATE_TYPE)
	uint8_t m_curAttackType = 0; // 현재 수행 중인 공격 종류
	uint8_t m_curAttackDir = 0;  // 현재 공격 조준 방향 (GENERAL_ATTACK_DIR_TYPE)
	uint8_t m_objType = 0;       // 캐릭터 타입 (GAME_OBJECT_TYPE)
	uint8_t m_stance = 0;        // 현재 자세 (GENERAL_STANCE_TYPE)
	FB_ENUMS::MOVE_DIRECTION_TYPE	 m_moveDir = FB_ENUMS::MOVE_DIRECTION_TYPE_FWD; // 현재 이동 방향
	bool    m_isLockOn = false;  // 현재 락온 상태 여부
	bool	m_isRunning = false; // 현재 달리기 상태 여부
	float   m_stateTimer = 0.0f; // 상태별 시간 추적용
	std::vector<StateChangeListener> m_listeners;
	std::vector<StanceChangeListener> m_stanceListeners;
};
