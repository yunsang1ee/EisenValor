#pragma once
#include "WalkState.h"

namespace Server {
	namespace Contents {
		class SoldierWalkState : public WalkState {
		private:
			Vec3						m_targetPos;
			bool						m_hasTarget;

		public:
			std::weak_ptr<GameObject>	m_ownerGeneral;
		public:
			virtual void Enter() override;
			virtual void Exit() override;

		public:
			virtual void Update(const float dt) override;

		public:		
			void SetTargetPos(const Vec3& targetPos) { m_hasTarget = true;  m_targetPos = targetPos; }
			void SetOwnerGeneral(std::weak_ptr<GameObject> owner) { m_ownerGeneral = owner; }
			std::shared_ptr<GameObject> GetOwner() noexcept { return m_ownerGeneral.lock(); }

		};
	}
}


