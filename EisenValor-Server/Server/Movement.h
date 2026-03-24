#pragma once

#include "Component.h"

namespace GameServer {
	namespace Contents {
		class Movement : public GameServer::Contents::Component {
		public:
			Movement();
			virtual ~Movement() = default;

		public:
			void SetMaxSpeed(const float maxSpeed) { m_maxSpeed = maxSpeed; }
			void SetAcceleration(const float acceleration) { m_acceleration = acceleration; }
			void SetRotationSpeed(const float rotationSpeed) { m_roatationSpeed = rotationSpeed; }

			void AddVelocity(const Vec3& velocity) { m_curVelocity += velocity; }
			void SetVelocity(const Vec3& velocity) { m_curVelocity = velocity; }

		public:
			const float GetMaxSpeed() const { return m_maxSpeed; }
			const float GetAcceleration() const { return m_acceleration; }
			const float GetRotationSpeed() const { return m_roatationSpeed; }
			const Vec3 GetCurVelocity() const { return m_curVelocity; }

		public:
			virtual void Update(const float dt) override;
			void UpdateRotation(const float dt);

		private:
			float	m_maxSpeed;
			float	m_acceleration;
			float	m_roatationSpeed;

			Vec3	m_curVelocity;			// 현재 속도
		};
	}
}

