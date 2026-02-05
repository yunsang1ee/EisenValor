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

public:
	explicit State(uint8_t type) : m_type(type) {}
	virtual ~State() = default;

	// 상태 로직 실행 시 핸들 받음
	virtual void Enter(HandleOf<FSMComponent> fsmHandle, float dt) = 0;
	virtual void Update(HandleOf<FSMComponent> fsmHandle, float dt) = 0;
	virtual void Exit(HandleOf<FSMComponent> fsmHandle, float dt) = 0;

	uint8_t GetStateType() const { return m_type; }
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
