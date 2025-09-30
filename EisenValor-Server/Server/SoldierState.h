#pragma once

#include "State.h"

namespace Server {
	namespace Contents {
		class GameObject;

		enum class SOLDIER_STATE_TYPE : uint8 {
			IDLE,
			RUN,
			ATTACK,

		};

		class SoldierIdleState : public State {
		private:
			Vec3 m_prevTargetPos;

		public:
			SoldierIdleState();
			virtual ~SoldierIdleState();

		public:
			virtual void Enter() override;
			virtual void Exit() override;

		public:
			virtual void Update(const float dt) override;
		};

		class SoldierRunState : public State {
		public:
			SoldierRunState();
			virtual ~SoldierRunState();

		public:
			virtual void Enter() override;
			virtual void Exit() override;
	
		public:
			virtual void Update(const float dt) override;

		};

		class SoldierAttackState : public State {
		public:
			SoldierAttackState();
			virtual ~SoldierAttackState();

		public:
			virtual void Enter() override;
			virtual void Exit() override;

		public:
			virtual void Update(const float dt) override;
		};

		//class SoldierTraceState : public State {
		//private:
		//	Vec3						m_targetPos;
		//	bool						m_hasTarget;
		//	Vec3						m_prevDir{ 0.f, 0.f, 0.f };

		//private:
		//	std::weak_ptr<GameObject>	m_ownerGeneral;

		//public:
		//	SoldierTraceState();
		//	virtual ~SoldierTraceState();

		//public:
		//	virtual void Enter() override;
		//	virtual void Exit() override;

		//public:
		//	virtual uint8 Update(const float dt) override;

		//public:
		//	void SetTargetPos(const Vec3& targetPos) { m_hasTarget = true;  m_targetPos = targetPos; }
		//	void SetOwnerGeneral(std::weak_ptr<GameObject> owner) { m_ownerGeneral = owner; }
		//	std::shared_ptr<GameObject> GetOwner() noexcept { return m_ownerGeneral.lock(); }

		//	inline Vec3 Lerp(const Vec3& a, const Vec3& b, float t)
		//	{
		//		// t ¡ô [0,1]
		//		return a * (1.0f - t) + b * t;
		//	}

		//	inline float LerpAngle(float a, float b, float t)
		//	{
		//		float diff = fmodf(b - a + 540.0f, 360.0f) - 180.0f;
		//		return a + diff * t;
		//	}

		//private:
		//	void Move(const float dt);
		//	void MoveByForce(const float dt);
		//};
	}
}
