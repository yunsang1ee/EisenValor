#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include "DenseList.h"

class FSMComponent;

 // Flyweight Pattern
 // 추상 부모 클래스
 // 데이터 없고 핸들 인자로 받음
class State {
protected:
	uint8_t m_type;
	bool	m_hasExitTime = false; // 애니메이션 종료 대기 여부
	uint8_t m_nextStateOnEnd = 0;  // 자동 전이

public:
	explicit State(uint8_t type) : m_type(type) {}
	virtual ~State() = default;

	// Enter/Exit는 이벤트이므로 dt가 필요 없음
	virtual void Enter(FSMComponent* fsm) = 0;
	virtual void Update(FSMComponent* fsm, float dt) = 0;
	virtual void Exit(FSMComponent* fsm) = 0;

	uint8_t GetStateType() const { return m_type; }

	// HasExitTime
	void SetHasExitTime(bool value) { m_hasExitTime = value; }
	bool HasExitTime() const { return m_hasExitTime; }

	// NextStateOnEnd
	void	SetNextStateOnEnd(uint8_t type) { m_nextStateOnEnd = type; }
	uint8_t GetNextStateOnEnd() const { return m_nextStateOnEnd; }
};


// 상태 로직 보관함
class StatePool {
public:
	static void Initialize();
	static void Release();
	static State* GetState(uint8_t type);

private:
	// 모든 인스턴스가 공유하는 정적 상태 창고
	static std::map<uint8_t, std::unique_ptr<State>> s_instanceMap;
};
