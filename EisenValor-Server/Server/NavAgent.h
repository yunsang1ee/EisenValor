#pragma once

#include "Component.h"

namespace GameServer {
	namespace Contents {
		class NavSystem;
		class NavAgent : public Component {
		public:
			explicit NavAgent(NavSystem* const navSystem);
			virtual ~NavAgent() = default;

		public:
			bool Init(const dtCrowdAgentParams& params);
			virtual void Update(const float dt) override final;

		public:
			void SetPlayerMode(bool isPlayer) { m_isPlayerMode = isPlayer; }
			void SetDestPos(const Vec3& destPos);
			int32 GetAgentIdx() const { return m_agentIdx; }
			void Teleport(const Vec3& destPos);
			
			void StopMove();
			void Remove();

			void SyncPosition(const Vec3& newPos, const Vec3& prevPos, float dt);
			void SetAsStaticObstacle();

		private:
			NavSystem*			m_navSystem;
			int32				m_agentIdx;
			dtCrowdAgentParams	m_params;

			Vec3				m_destPos;
			bool				m_hasTarget;

			bool				m_isPlayerMode;

		};
	}
}

